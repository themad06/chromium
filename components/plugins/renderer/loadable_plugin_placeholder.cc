// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/plugins/renderer/loadable_plugin_placeholder.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/json/string_escape.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "gin/object_template_builder.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/re2/re2/re2.h"

using base::UserMetricsAction;
using blink::WebElement;
using blink::WebLocalFrame;
using blink::WebMouseEvent;
using blink::WebNode;
using blink::WebPlugin;
using blink::WebPluginContainer;
using blink::WebPluginParams;
using blink::WebScriptSource;
using blink::WebURLRequest;
using content::PluginInstanceThrottler;
using content::PluginPowerSaverMode;
using content::RenderThread;

namespace plugins {

#if defined(ENABLE_PLUGINS)
void LoadablePluginPlaceholder::BlockForPowerSaverPoster() {
  DCHECK(!is_blocked_for_power_saver_poster_);
  is_blocked_for_power_saver_poster_ = true;

  render_frame()->RegisterPeripheralPlugin(
      GURL(GetPluginParams().url).GetOrigin(),
      base::Bind(&LoadablePluginPlaceholder::DisablePowerSaverForInstance,
                 weak_factory_.GetWeakPtr(),
                 PluginInstanceThrottler::UNTHROTTLE_METHOD_BY_WHITELIST));
}
#endif

LoadablePluginPlaceholder::LoadablePluginPlaceholder(
    content::RenderFrame* render_frame,
    WebLocalFrame* frame,
    const WebPluginParams& params,
    const std::string& html_data,
    GURL placeholderDataUrl)
    : PluginPlaceholder(render_frame,
                        frame,
                        params,
                        html_data,
                        placeholderDataUrl),
      is_blocked_for_background_tab_(false),
      is_blocked_for_prerendering_(false),
      is_blocked_for_power_saver_poster_(false),
      power_saver_mode_(PluginPowerSaverMode::POWER_SAVER_MODE_ESSENTIAL),
      allow_loading_(false),
      placeholder_was_replaced_(false),
      hidden_(false),
      finished_loading_(false),
      weak_factory_(this) {
}

LoadablePluginPlaceholder::~LoadablePluginPlaceholder() {
#if defined(ENABLE_PLUGINS)
  if (!placeholder_was_replaced_ && !is_blocked_for_prerendering_ &&
      power_saver_mode_ != PluginPowerSaverMode::POWER_SAVER_MODE_ESSENTIAL) {
    PluginInstanceThrottler::RecordUnthrottleMethodMetric(
        PluginInstanceThrottler::UNTHROTTLE_METHOD_NEVER);
  }
#endif
}

#if defined(ENABLE_PLUGINS)
void LoadablePluginPlaceholder::DisablePowerSaverForInstance(
    PluginInstanceThrottler::PowerSaverUnthrottleMethod method) {
  if (power_saver_mode_ == PluginPowerSaverMode::POWER_SAVER_MODE_ESSENTIAL)
    return;

  power_saver_mode_ = PluginPowerSaverMode::POWER_SAVER_MODE_ESSENTIAL;
  PluginInstanceThrottler::RecordUnthrottleMethodMetric(method);
  if (is_blocked_for_power_saver_poster_) {
    is_blocked_for_power_saver_poster_ = false;
    if (!LoadingBlocked())
      LoadPlugin();
  }
}
#endif

gin::ObjectTemplateBuilder LoadablePluginPlaceholder::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<PluginPlaceholder>::GetObjectTemplateBuilder(isolate)
      .SetMethod("load", &LoadablePluginPlaceholder::LoadCallback)
      .SetMethod("hide", &LoadablePluginPlaceholder::HideCallback)
      .SetMethod("didFinishLoading",
                 &LoadablePluginPlaceholder::DidFinishLoadingCallback);
}

void LoadablePluginPlaceholder::ReplacePlugin(WebPlugin* new_plugin) {
  CHECK(plugin());
  if (!new_plugin)
    return;
  WebPluginContainer* container = plugin()->container();
  // Set the new plug-in on the container before initializing it.
  container->setPlugin(new_plugin);
  // Save the element in case the plug-in is removed from the page during
  // initialization.
  WebElement element = container->element();
  if (!new_plugin->initialize(container)) {
    // We couldn't initialize the new plug-in. Restore the old one and abort.
    container->setPlugin(plugin());
    return;
  }

  // The plug-in has been removed from the page. Destroy the old plug-in. We
  // will be destroyed as soon as V8 garbage collects us.
  if (!element.pluginContainer()) {
    plugin()->destroy();
    return;
  }

  // During initialization, the new plug-in might have replaced itself in turn
  // with another plug-in. Make sure not to use the passed in |new_plugin| after
  // this point.
  new_plugin = container->plugin();

  plugin()->RestoreTitleText();
  container->invalidate();
  container->reportGeometry();
  plugin()->ReplayReceivedData(new_plugin);
  plugin()->destroy();

  placeholder_was_replaced_ = true;
}

void LoadablePluginPlaceholder::HidePlugin() {
  hidden_ = true;
  if (!plugin())
    return;
  WebPluginContainer* container = plugin()->container();
  WebElement element = container->element();
  element.setAttribute("style", "display: none;");
  // If we have a width and height, search for a parent (often <div>) with the
  // same dimensions. If we find such a parent, hide that as well.
  // This makes much more uncovered page content usable (including clickable)
  // as opposed to merely visible.
  // TODO(cevans) -- it's a foul heurisitc but we're going to tolerate it for
  // now for these reasons:
  // 1) Makes the user experience better.
  // 2) Foulness is encapsulated within this single function.
  // 3) Confidence in no fasle positives.
  // 4) Seems to have a good / low false negative rate at this time.
  if (element.hasAttribute("width") && element.hasAttribute("height")) {
    std::string width_str("width:[\\s]*");
    width_str += element.getAttribute("width").utf8().data();
    if (EndsWith(width_str, "px", false)) {
      width_str = width_str.substr(0, width_str.length() - 2);
    }
    base::TrimWhitespace(width_str, base::TRIM_TRAILING, &width_str);
    width_str += "[\\s]*px";
    std::string height_str("height:[\\s]*");
    height_str += element.getAttribute("height").utf8().data();
    if (EndsWith(height_str, "px", false)) {
      height_str = height_str.substr(0, height_str.length() - 2);
    }
    base::TrimWhitespace(height_str, base::TRIM_TRAILING, &height_str);
    height_str += "[\\s]*px";
    WebNode parent = element;
    while (!parent.parentNode().isNull()) {
      parent = parent.parentNode();
      if (!parent.isElementNode())
        continue;
      element = parent.toConst<WebElement>();
      if (element.hasAttribute("style")) {
        std::string style_str = element.getAttribute("style").utf8();
        if (RE2::PartialMatch(style_str, width_str) &&
            RE2::PartialMatch(style_str, height_str))
          element.setAttribute("style", "display: none;");
      }
    }
  }
}

void LoadablePluginPlaceholder::SetMessage(const base::string16& message) {
  message_ = message;
  if (finished_loading_)
    UpdateMessage();
}

void LoadablePluginPlaceholder::UpdateMessage() {
  if (!plugin())
    return;
  std::string script =
      "window.setMessage(" + base::GetQuotedJSONString(message_) + ")";
  plugin()->web_view()->mainFrame()->executeScript(
      WebScriptSource(base::UTF8ToUTF16(script)));
}

void LoadablePluginPlaceholder::ShowContextMenu(const WebMouseEvent& event) {
  // Does nothing by default. Will be overridden if a specific browser wants
  // a context menu.
  return;
}

void LoadablePluginPlaceholder::WasShown() {
  if (is_blocked_for_background_tab_) {
    is_blocked_for_background_tab_ = false;
    if (!LoadingBlocked())
      LoadPlugin();
  }
}

void LoadablePluginPlaceholder::OnLoadBlockedPlugins(
    const std::string& identifier) {
  if (!identifier.empty() && identifier != identifier_)
    return;

  RenderThread::Get()->RecordAction(UserMetricsAction("Plugin_Load_UI"));
  LoadPlugin();
}

void LoadablePluginPlaceholder::OnSetIsPrerendering(bool is_prerendering) {
  // Prerendering can only be enabled prior to a RenderView's first navigation,
  // so no BlockedPlugin should see the notification that enables prerendering.
  DCHECK(!is_prerendering);
  if (is_blocked_for_prerendering_) {
    is_blocked_for_prerendering_ = false;
    if (!LoadingBlocked())
      LoadPlugin();
  }
}

void LoadablePluginPlaceholder::LoadPlugin() {
  // This is not strictly necessary but is an important defense in case the
  // event propagation changes between "close" vs. "click-to-play".
  if (hidden_)
    return;
  if (!plugin())
    return;
  if (!allow_loading_) {
    NOTREACHED();
    return;
  }

  // TODO(mmenke):  In the case of prerendering, feed into
  //                ChromeContentRendererClient::CreatePlugin instead, to
  //                reduce the chance of future regressions.
  scoped_ptr<PluginInstanceThrottler> throttler;
#if defined(ENABLE_PLUGINS)
  throttler = PluginInstanceThrottler::Get(
      render_frame(), GetPluginParams().url, power_saver_mode_);
#endif
  WebPlugin* plugin = render_frame()->CreatePlugin(
      GetFrame(), plugin_info_, GetPluginParams(), throttler.Pass());
  ReplacePlugin(plugin);
}

void LoadablePluginPlaceholder::LoadCallback() {
  RenderThread::Get()->RecordAction(UserMetricsAction("Plugin_Load_Click"));
#if defined(ENABLE_PLUGINS)
  // If the user specifically clicks on the plug-in content's placeholder,
  // disable power saver throttling for this instance.
  DisablePowerSaverForInstance(
      PluginInstanceThrottler::UNTHROTTLE_METHOD_BY_CLICK);
#endif
  LoadPlugin();
}

void LoadablePluginPlaceholder::HideCallback() {
  RenderThread::Get()->RecordAction(UserMetricsAction("Plugin_Hide_Click"));
  HidePlugin();
}

void LoadablePluginPlaceholder::DidFinishLoadingCallback() {
  finished_loading_ = true;
  if (message_.length() > 0)
    UpdateMessage();
}

void LoadablePluginPlaceholder::SetPluginInfo(
    const content::WebPluginInfo& plugin_info) {
  plugin_info_ = plugin_info;
}

const content::WebPluginInfo& LoadablePluginPlaceholder::GetPluginInfo() const {
  return plugin_info_;
}

void LoadablePluginPlaceholder::SetIdentifier(const std::string& identifier) {
  identifier_ = identifier;
}

bool LoadablePluginPlaceholder::LoadingBlocked() const {
  DCHECK(allow_loading_);
  return is_blocked_for_background_tab_ || is_blocked_for_power_saver_poster_ ||
         is_blocked_for_prerendering_;
}

}  // namespace plugins
