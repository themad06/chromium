// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_WINDOW_SELECTOR_ITEM_H_
#define ASH_WM_OVERVIEW_WINDOW_SELECTOR_ITEM_H_

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"
#include "ui/gfx/rect.h"

namespace aura {
class Window;
}

namespace views {
class Widget;
}

namespace ash {
class TransparentActivateWindowButton;

// This class represents an item in overview mode. An item can have one or more
// windows, of which only one can be activated by keyboard (i.e. alt+tab) but
// any can be selected with a pointer (touch or mouse).
class WindowSelectorItem {
 public:
  WindowSelectorItem();
  virtual ~WindowSelectorItem();

  // The time for the close buttons and labels to fade in when initially shown
  // on entering overview mode.
  static const int kFadeInMilliseconds;

  // Returns the root window on which this item is shown.
  virtual aura::Window* GetRootWindow() = 0;

  // Returns true if the window selector item has |window| as a selectable
  // window.
  virtual bool HasSelectableWindow(const aura::Window* window) = 0;

  // Returns true if |target| is contained in this WindowSelectorItem.
  virtual bool Contains(const aura::Window* target) = 0;

  // Restores |window| on exiting window overview rather than returning it
  // to its previous state.
  virtual void RestoreWindowOnExit(aura::Window* window) = 0;

  // Returns the |window| to activate on selecting of this item.
  virtual aura::Window* SelectionWindow() = 0;

  // Removes |window| from this item. Check empty() after calling this to see
  // if the entire item is now empty.
  virtual void RemoveWindow(const aura::Window* window) = 0;

  // Returns true if this item has no more selectable windows (i.e. after
  // calling RemoveWindow for the last contained window).
  virtual bool empty() const = 0;

  // Dispatched before beginning window overview. This will do any necessary
  // one time actions such as restoring minimized windows.
  virtual void PrepareForOverview() = 0;

  // Sets the bounds of this window selector item to |target_bounds| in the
  // |root_window| root window.
  void SetBounds(aura::Window* root_window,
                 const gfx::Rect& target_bounds,
                 bool animate);

  // Recomputes the positions for the windows in this selection item. This is
  // dispatched when the bounds of a window change.
  void RecomputeWindowTransforms();

  const gfx::Rect& bounds() { return bounds_; }
  const gfx::Rect& target_bounds() { return target_bounds_; }

 protected:
  // Sets the bounds of this selector's items to |target_bounds| in
  // |root_window|. If |animate| the windows are animated from their current
  // location.
  virtual void SetItemBounds(aura::Window* root_window,
                             const gfx::Rect& target_bounds,
                             bool animate) = 0;

  // Sets the bounds used by the selector item's windows.
  void set_bounds(const gfx::Rect& bounds) { bounds_ = bounds; }

 private:
  friend class WindowSelectorTest;

  // Creates a label to display under the window selector item.
  void UpdateWindowLabels(const gfx::Rect& target_bounds,
                          aura::Window* root_window,
                          bool animate);

  // The root window this item is being displayed on.
  aura::Window* root_window_;

  // The target bounds this selector item is fit within.
  gfx::Rect target_bounds_;

  // The actual bounds of the window(s) for this item. The aspect ratio of
  // window(s) are maintained so they may not fill the target_bounds_.
  gfx::Rect bounds_;

  // True if running SetItemBounds. This prevents recursive calls resulting from
  // the bounds update when calling ::wm::RecreateWindowLayers to copy
  // a window layer for display on another monitor.
  bool in_bounds_update_;

  // Label under the window displaying its active tab name.
  scoped_ptr<views::Widget> window_label_;

  // Transparent window on top of the real windows in the overview that
  // activates them on click or tap.
  scoped_ptr<TransparentActivateWindowButton> activate_window_button_;

  DISALLOW_COPY_AND_ASSIGN(WindowSelectorItem);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_WINDOW_SELECTOR_ITEM_H_
