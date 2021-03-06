// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_preview_dialog_controller.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/auto_reset.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/chrome_extension_web_contents_observer.h"
#include "chrome/browser/plugins/chrome_plugin_service_filter.h"
#include "chrome/browser/printing/print_view_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/browser/ui/webui/chrome_web_contents_handler.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/url_constants.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/webplugininfo.h"
#include "extensions/browser/guest_view/guest_view_base.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

using content::NavigationController;
using content::WebContents;
using content::WebUIMessageHandler;

namespace {

void EnableInternalPDFPluginForContents(WebContents* preview_dialog) {
  // Always enable the internal PDF plugin for the print preview page.
  base::FilePath pdf_plugin_path;
  if (!PathService::Get(chrome::FILE_PDF_PLUGIN, &pdf_plugin_path))
    return;

  content::WebPluginInfo pdf_plugin;
  if (!content::PluginService::GetInstance()->GetPluginInfoByPath(
      pdf_plugin_path, &pdf_plugin))
    return;

  ChromePluginServiceFilter::GetInstance()->OverridePluginForFrame(
      preview_dialog->GetRenderProcessHost()->GetID(),
      preview_dialog->GetMainFrame()->GetRoutingID(),
      GURL(), pdf_plugin);
}

// A ui::WebDialogDelegate that specifies the print preview dialog appearance.
class PrintPreviewDialogDelegate : public ui::WebDialogDelegate {
 public:
  explicit PrintPreviewDialogDelegate(WebContents* initiator);
  ~PrintPreviewDialogDelegate() override;

  ui::ModalType GetDialogModalType() const override;
  base::string16 GetDialogTitle() const override;
  GURL GetDialogContentURL() const override;
  void GetWebUIMessageHandlers(
      std::vector<WebUIMessageHandler*>* handlers) const override;
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;
  void OnDialogClosed(const std::string& json_retval) override;
  void OnCloseContents(WebContents* source, bool* out_close_dialog) override;
  bool ShouldShowDialogTitle() const override;

 private:
  WebContents* initiator_;

  DISALLOW_COPY_AND_ASSIGN(PrintPreviewDialogDelegate);
};

PrintPreviewDialogDelegate::PrintPreviewDialogDelegate(WebContents* initiator)
    : initiator_(initiator) {
}

PrintPreviewDialogDelegate::~PrintPreviewDialogDelegate() {
}

ui::ModalType PrintPreviewDialogDelegate::GetDialogModalType() const {
  // Not used, returning dummy value.
  NOTREACHED();
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 PrintPreviewDialogDelegate::GetDialogTitle() const {
  // Only used on Windows? UI folks prefer no title.
  return base::string16();
}

GURL PrintPreviewDialogDelegate::GetDialogContentURL() const {
  return GURL(chrome::kChromeUIPrintURL);
}

void PrintPreviewDialogDelegate::GetWebUIMessageHandlers(
    std::vector<WebUIMessageHandler*>* /* handlers */) const {
  // PrintPreviewUI adds its own message handlers.
}

void PrintPreviewDialogDelegate::GetDialogSize(gfx::Size* size) const {
  DCHECK(size);
  const gfx::Size kMinDialogSize(800, 480);
  const int kBorder = 25;
  *size = kMinDialogSize;

  web_modal::WebContentsModalDialogHost* host = NULL;
  content::WebContents* outermost_web_contents = initiator_;
  const extensions::GuestViewBase* guest_view =
      extensions::GuestViewBase::FromWebContents(outermost_web_contents);
  while (guest_view && guest_view->attached()) {
    outermost_web_contents = guest_view->embedder_web_contents();
    guest_view =
        extensions::GuestViewBase::FromWebContents(outermost_web_contents);
  }
  Browser* browser = chrome::FindBrowserWithWebContents(outermost_web_contents);
  if (browser)
    host = browser->window()->GetWebContentsModalDialogHost();

  if (host) {
    size->SetToMax(host->GetMaximumDialogSize());
    size->Enlarge(-2 * kBorder, -kBorder);
  } else {
    size->SetToMax(outermost_web_contents->GetContainerBounds().size());
    size->Enlarge(-2 * kBorder, -2 * kBorder);
  }

#if defined(OS_MACOSX)
  // Limit the maximum size on MacOS X.
  // http://crbug.com/105815
  const gfx::Size kMaxDialogSize(1000, 660);
  size->SetToMin(kMaxDialogSize);
#endif
}

std::string PrintPreviewDialogDelegate::GetDialogArgs() const {
  return std::string();
}

void PrintPreviewDialogDelegate::OnDialogClosed(
    const std::string& /* json_retval */) {
}

void PrintPreviewDialogDelegate::OnCloseContents(WebContents* /* source */,
                                                 bool* out_close_dialog) {
  if (out_close_dialog)
    *out_close_dialog = true;
}

bool PrintPreviewDialogDelegate::ShouldShowDialogTitle() const {
  return false;
}

}  // namespace

namespace printing {

PrintPreviewDialogController::PrintPreviewDialogController()
    : waiting_for_new_preview_page_(false),
      is_creating_print_preview_dialog_(false) {
}

// static
PrintPreviewDialogController* PrintPreviewDialogController::GetInstance() {
  if (!g_browser_process)
    return NULL;
  return g_browser_process->print_preview_dialog_controller();
}

// static
void PrintPreviewDialogController::PrintPreview(WebContents* initiator) {
  if (initiator->ShowingInterstitialPage())
    return;

  PrintPreviewDialogController* dialog_controller = GetInstance();
  if (!dialog_controller)
    return;
  if (!dialog_controller->GetOrCreatePreviewDialog(initiator))
    PrintViewManager::FromWebContents(initiator)->PrintPreviewDone();
}

WebContents* PrintPreviewDialogController::GetOrCreatePreviewDialog(
    WebContents* initiator) {
  DCHECK(initiator);

  // Get the print preview dialog for |initiator|.
  WebContents* preview_dialog = GetPrintPreviewForContents(initiator);
  if (!preview_dialog)
    return CreatePrintPreviewDialog(initiator);

  // Show the initiator holding the existing preview dialog.
  initiator->GetDelegate()->ActivateContents(initiator);
  return preview_dialog;
}

WebContents* PrintPreviewDialogController::GetPrintPreviewForContents(
    WebContents* contents) const {
  // |preview_dialog_map_| is keyed by the preview dialog, so if find()
  // succeeds, then |contents| is the preview dialog.
  PrintPreviewDialogMap::const_iterator it = preview_dialog_map_.find(contents);
  if (it != preview_dialog_map_.end())
    return contents;

  for (it = preview_dialog_map_.begin();
       it != preview_dialog_map_.end();
       ++it) {
    // If |contents| is an initiator.
    if (contents == it->second) {
      // Return the associated preview dialog.
      return it->first;
    }
  }
  return NULL;
}

WebContents* PrintPreviewDialogController::GetInitiator(
    WebContents* preview_dialog) {
  PrintPreviewDialogMap::iterator it = preview_dialog_map_.find(preview_dialog);
  return (it != preview_dialog_map_.end()) ? it->second : NULL;
}

void PrintPreviewDialogController::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  if (type == content::NOTIFICATION_RENDERER_PROCESS_CLOSED) {
    OnRendererProcessClosed(
        content::Source<content::RenderProcessHost>(source).ptr());
  } else if (type == content::NOTIFICATION_WEB_CONTENTS_DESTROYED) {
    OnWebContentsDestroyed(content::Source<WebContents>(source).ptr());
  } else {
    DCHECK_EQ(content::NOTIFICATION_NAV_ENTRY_COMMITTED, type);
    WebContents* contents =
        content::Source<NavigationController>(source)->GetWebContents();
    OnNavEntryCommitted(
        contents,
        content::Details<content::LoadCommittedDetails>(details).ptr());
  }
}

void PrintPreviewDialogController::ForEachPreviewDialog(
    base::Callback<void(content::WebContents*)> callback) {
  for (PrintPreviewDialogMap::const_iterator it = preview_dialog_map_.begin();
       it != preview_dialog_map_.end();
       ++it) {
    callback.Run(it->first);
  }
}

// static
bool PrintPreviewDialogController::IsPrintPreviewDialog(WebContents* contents) {
  return IsPrintPreviewURL(contents->GetURL());
}

// static
bool PrintPreviewDialogController::IsPrintPreviewURL(const GURL& url) {
  return (url.SchemeIs(content::kChromeUIScheme) &&
          url.host() == chrome::kChromeUIPrintHost);
}

void PrintPreviewDialogController::EraseInitiatorInfo(
    WebContents* preview_dialog) {
  PrintPreviewDialogMap::iterator it = preview_dialog_map_.find(preview_dialog);
  if (it == preview_dialog_map_.end())
    return;

  RemoveObservers(it->second);
  preview_dialog_map_[preview_dialog] = NULL;
}

PrintPreviewDialogController::~PrintPreviewDialogController() {}

void PrintPreviewDialogController::OnRendererProcessClosed(
    content::RenderProcessHost* rph) {
  // Store contents in a vector and deal with them after iterating through
  // |preview_dialog_map_| because RemoveFoo() can change |preview_dialog_map_|.
  std::vector<WebContents*> closed_initiators;
  std::vector<WebContents*> closed_preview_dialogs;
  for (PrintPreviewDialogMap::iterator iter = preview_dialog_map_.begin();
       iter != preview_dialog_map_.end(); ++iter) {
    WebContents* preview_dialog = iter->first;
    WebContents* initiator = iter->second;
    if (preview_dialog->GetRenderProcessHost() == rph) {
      closed_preview_dialogs.push_back(preview_dialog);
    } else if (initiator &&
               initiator->GetRenderProcessHost() == rph) {
      closed_initiators.push_back(initiator);
    }
  }

  for (size_t i = 0; i < closed_preview_dialogs.size(); ++i) {
    RemovePreviewDialog(closed_preview_dialogs[i]);
    if (content::WebUI* web_ui = closed_preview_dialogs[i]->GetWebUI()) {
      PrintPreviewUI* print_preview_ui =
          static_cast<PrintPreviewUI*>(web_ui->GetController());
      if (print_preview_ui)
        print_preview_ui->OnPrintPreviewDialogClosed();
    }
  }

  for (size_t i = 0; i < closed_initiators.size(); ++i)
    RemoveInitiator(closed_initiators[i]);
}

void PrintPreviewDialogController::OnWebContentsDestroyed(
    WebContents* contents) {
  WebContents* preview_dialog = GetPrintPreviewForContents(contents);
  if (!preview_dialog) {
    NOTREACHED();
    return;
  }

  if (contents == preview_dialog)
    RemovePreviewDialog(contents);
  else
    RemoveInitiator(contents);
}

void PrintPreviewDialogController::OnNavEntryCommitted(
    WebContents* contents, content::LoadCommittedDetails* details) {
  WebContents* preview_dialog = GetPrintPreviewForContents(contents);
  if (!preview_dialog) {
    NOTREACHED();
    return;
  }

  if (contents == preview_dialog) {
    // Preview dialog navigated.
    if (details) {
      ui::PageTransition transition_type =
          details->entry->GetTransitionType();
      content::NavigationType nav_type = details->type;

      // New |preview_dialog| is created. Don't update/erase map entry.
      if (waiting_for_new_preview_page_ &&
          transition_type == ui::PAGE_TRANSITION_AUTO_TOPLEVEL &&
          nav_type == content::NAVIGATION_TYPE_NEW_PAGE) {
        waiting_for_new_preview_page_ = false;
        SaveInitiatorTitle(preview_dialog);
        return;
      }

      // Cloud print sign-in causes a reload.
      if (!waiting_for_new_preview_page_ &&
          transition_type == ui::PAGE_TRANSITION_RELOAD &&
          nav_type == content::NAVIGATION_TYPE_EXISTING_PAGE &&
          IsPrintPreviewURL(details->previous_url)) {
        return;
      }
    }
    NOTREACHED();
    return;
  }

  RemoveInitiator(contents);
}

WebContents* PrintPreviewDialogController::CreatePrintPreviewDialog(
    WebContents* initiator) {
  base::AutoReset<bool> auto_reset(&is_creating_print_preview_dialog_, true);

  // The dialog delegates are deleted when the dialog is closed.
  ConstrainedWebDialogDelegate* web_dialog_delegate =
      ShowConstrainedWebDialog(initiator->GetBrowserContext(),
                               new PrintPreviewDialogDelegate(initiator),
                               initiator);

  WebContents* preview_dialog = web_dialog_delegate->GetWebContents();
  EnableInternalPDFPluginForContents(preview_dialog);
  PrintViewManager::CreateForWebContents(preview_dialog);
  extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
      preview_dialog);

  // Add an entry to the map.
  preview_dialog_map_[preview_dialog] = initiator;
  waiting_for_new_preview_page_ = true;

  AddObservers(initiator);
  AddObservers(preview_dialog);

  return preview_dialog;
}

void PrintPreviewDialogController::SaveInitiatorTitle(
    WebContents* preview_dialog) {
  WebContents* initiator = GetInitiator(preview_dialog);
  if (initiator && preview_dialog->GetWebUI()) {
    PrintPreviewUI* print_preview_ui = static_cast<PrintPreviewUI*>(
        preview_dialog->GetWebUI()->GetController());
    print_preview_ui->SetInitiatorTitle(
        PrintViewManager::FromWebContents(initiator)->RenderSourceName());
  }
}

void PrintPreviewDialogController::AddObservers(WebContents* contents) {
  registrar_.Add(this, content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
                 content::Source<WebContents>(contents));
  registrar_.Add(this, content::NOTIFICATION_NAV_ENTRY_COMMITTED,
      content::Source<NavigationController>(&contents->GetController()));

  // Multiple sites may share the same RenderProcessHost, so check if this
  // notification has already been added.
  content::Source<content::RenderProcessHost> rph_source(
      contents->GetRenderProcessHost());
  if (!registrar_.IsRegistered(this,
      content::NOTIFICATION_RENDERER_PROCESS_CLOSED, rph_source)) {
    registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_CLOSED,
                   rph_source);
  }
}

void PrintPreviewDialogController::RemoveObservers(WebContents* contents) {
  registrar_.Remove(this, content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
                    content::Source<WebContents>(contents));
  registrar_.Remove(this, content::NOTIFICATION_NAV_ENTRY_COMMITTED,
      content::Source<NavigationController>(&contents->GetController()));

  // Multiple sites may share the same RenderProcessHost, so check if this
  // notification has already been added.
  content::Source<content::RenderProcessHost> rph_source(
      contents->GetRenderProcessHost());
  if (registrar_.IsRegistered(this,
      content::NOTIFICATION_RENDERER_PROCESS_CLOSED, rph_source)) {
    registrar_.Remove(this, content::NOTIFICATION_RENDERER_PROCESS_CLOSED,
                      rph_source);
  }
}

void PrintPreviewDialogController::RemoveInitiator(
    WebContents* initiator) {
  WebContents* preview_dialog = GetPrintPreviewForContents(initiator);
  DCHECK(preview_dialog);
  // Update the map entry first, so when the print preview dialog gets destroyed
  // and reaches RemovePreviewDialog(), it does not attempt to also remove the
  // initiator's observers.
  preview_dialog_map_[preview_dialog] = NULL;
  RemoveObservers(initiator);

  PrintViewManager::FromWebContents(initiator)->PrintPreviewDone();

  // initiator is closed. Close the print preview dialog too.
  if (content::WebUI* web_ui = preview_dialog->GetWebUI()) {
    PrintPreviewUI* print_preview_ui =
        static_cast<PrintPreviewUI*>(web_ui->GetController());
    if (print_preview_ui)
      print_preview_ui->OnInitiatorClosed();
  }
}

void PrintPreviewDialogController::RemovePreviewDialog(
    WebContents* preview_dialog) {
  // Remove the initiator's observers before erasing the mapping.
  WebContents* initiator = GetInitiator(preview_dialog);
  if (initiator) {
    RemoveObservers(initiator);
    PrintViewManager::FromWebContents(initiator)->PrintPreviewDone();
  }

  // Print preview WebContents is destroyed. Notify |PrintPreviewUI| to abort
  // the initiator preview request.
  if (content::WebUI* web_ui = preview_dialog->GetWebUI()) {
    PrintPreviewUI* print_preview_ui =
        static_cast<PrintPreviewUI*>(web_ui->GetController());
    if (print_preview_ui)
      print_preview_ui->OnPrintPreviewDialogDestroyed();
  }

  preview_dialog_map_.erase(preview_dialog);
  RemoveObservers(preview_dialog);
}

}  // namespace printing
