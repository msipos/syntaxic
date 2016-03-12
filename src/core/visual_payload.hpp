#ifndef SYNTAXIC_CORE_VISUAL_PAYLOAD_HPP
#define SYNTAXIC_CORE_VISUAL_PAYLOAD_HPP

class FlowGrid;

/** This is a feedback from the visual layer that may or may not be used by the logical layer. */
struct VisualPayload {
  int page_size;

  /** May be null. */
  FlowGrid* flow_grid;

  /** Is navigation mode active? */
  bool navigation_mode;

  inline VisualPayload() : page_size(10), flow_grid(nullptr), navigation_mode(false) {}
  inline VisualPayload(int ps, FlowGrid* fg, bool nm) : page_size(ps), flow_grid(fg), navigation_mode(nm) {}
};

#endif