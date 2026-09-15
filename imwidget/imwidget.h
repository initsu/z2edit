#ifndef Z2UTIL_IMWIDGET_IMWIDGET_H
#define Z2UTIL_IMWIDGET_IMWIDGET_H


inline int UniqueID() {
    static int id;
    return ++id;
}

class ImWindowBase {
  public:
    explicit ImWindowBase()
      : ImWindowBase(true) {}
    explicit ImWindowBase(bool visible)
      : ImWindowBase(visible, true) {}
    explicit ImWindowBase(bool visible, bool want_dispose)
      : id_(UniqueID()), visible_(visible), want_dispose_(want_dispose),
        focus_next_(false) {}
    virtual ~ImWindowBase() {}

    virtual bool Draw() { return false; }
    virtual void Refresh() {}
    inline bool& visible() { return visible_; }
    inline void set_visible(bool v) { visible_ = v; }
    inline bool want_dispose() { return want_dispose_; }
    inline void set_want_dispose(bool v) { want_dispose_ = v; }
    // Requests that the widget be brought to the front on the next frame that
    // it is drawn.  Drawing code should consume this via consume_focus_next()
    // and issue an ImGui::SetNextWindowFocus() to raise the window.
    inline void set_focus_next(bool v) { focus_next_ = v; }
    inline bool consume_focus_next() {
        bool v = focus_next_;
        focus_next_ = false;
        return v;
    }
  protected:
    int id_;
    bool visible_;
    bool want_dispose_;
    bool focus_next_;
};

#endif // Z2UTIL_IMWIDGET_IMWIDGET_H
