// =============================================================================
// EXAMPLE_BRIDGE.CPP - Demonstrates All Bridge Features
// =============================================================================
// NOTE: Functions prefixed with "demo_" to avoid conflicts with existing
// functions

#include "bridge.h"
#include <cmath>
#include <cstring>

// =============================================================================
// 1. FUNCTIONS - Basic function export
// =============================================================================

BRIDGE int demo_add(int a, int b) { return a + b; }

BRIDGE double demo_multiply(double a, double b) { return a * b; }

BRIDGE str demo_greet(str name) {
  return RET("Hello, " + std::string(name ? name : "World") + "!");
}

// =============================================================================
// 2. VARIABLES - Shared state with auto getter/setter
// =============================================================================

BRIDGE_VAR(int, demo_counter, 0);
BRIDGE_VAR(double, demo_pi, 3.14159);
BRIDGE_VAR_STR(demo_user, "Guest");

// =============================================================================
// 3. ENUMS - Named constants
// =============================================================================

BRIDGE_ENUM_3(DemoColor, Red, Green, Blue);
BRIDGE_ENUM_4(DemoStatus, Idle, Running, Done, Failed);

// =============================================================================
// 4. STRUCTS - Data containers (returned as JSON)
// =============================================================================

BRIDGE_STRUCT_2(DemoPoint, int, x, int, y);
BRIDGE_STRUCT_3(DemoVec3, double, x, double, y, double, z);

// =============================================================================
// 5. CLASSES - Object-oriented with handles
// =============================================================================

class DemoCalc {
public:
  int value = 0;
  int getValue() { return value; }
  int addTo(int n) {
    value += n;
    return value;
  }
  void reset() { value = 0; }
};

static std::unordered_map<int, DemoCalc *> _demo_calcs;
static int _demo_calc_id = 0;

BRIDGE int DemoCalc_new() {
  int id = ++_demo_calc_id;
  _demo_calcs[id] = new DemoCalc();
  return id;
}

BRIDGE void DemoCalc_delete(int handle) {
  if (_demo_calcs.count(handle)) {
    delete _demo_calcs[handle];
    _demo_calcs.erase(handle);
  }
}

BRIDGE int DemoCalc_getValue(int handle) {
  if (!_demo_calcs.count(handle))
    return 0;
  return _demo_calcs[handle]->getValue();
}

BRIDGE int DemoCalc_addTo(int handle, int n) {
  if (!_demo_calcs.count(handle))
    return 0;
  return _demo_calcs[handle]->addTo(n);
}

BRIDGE void DemoCalc_reset(int handle) {
  if (_demo_calcs.count(handle))
    _demo_calcs[handle]->reset();
}

// =============================================================================
// 6. EXCEPTIONS - Error handling
// =============================================================================

BRIDGE_SAFE int demo_divide(int a, int b) {
  if (b == 0)
    BRIDGE_THROW("Division by zero");
  return a / b;
}

BRIDGE_SAFE double demo_sqrt(double n) {
  if (n < 0)
    BRIDGE_THROW("Cannot sqrt negative number");
  return std::sqrt(n);
}

// =============================================================================
// 7. CALLBACKS - Async notification
// =============================================================================

BRIDGE_CALLBACK(DemoProgress, int);
BRIDGE_CALLBACK_STR(DemoMessage);

BRIDGE void demo_start_task(int taskId) {
  call_DemoProgress(0);
  call_DemoProgress(50);
  call_DemoProgress(100);
  call_DemoMessage("Task complete!");
}

// =============================================================================
// 8. MODULES - Grouped by naming convention
// =============================================================================

BRIDGE double DemoMath_sin(double x) { return std::sin(x); }
BRIDGE double DemoMath_cos(double x) { return std::cos(x); }

BRIDGE int DemoStr_length(str s) { return s ? (int)strlen(s) : 0; }
BRIDGE str DemoStr_upper(str s) {
  if (!s)
    return "";
  std::string result = s;
  for (char &c : result)
    if (c >= 'a' && c <= 'z')
      c -= 32;
  return RET(result);
}

// =============================================================================
// 9. INHERITANCE - Shape example
// =============================================================================

class DemoShape {
public:
  virtual double area() = 0;
  virtual ~DemoShape() = default;
};

class DemoCircle : public DemoShape {
public:
  double radius;
  DemoCircle(double r) : radius(r) {}
  double area() override { return 3.14159 * radius * radius; }
};

static std::unordered_map<int, DemoCircle *> _demo_circles;
static int _demo_circle_id = 0;

BRIDGE int DemoCircle_new(double radius) {
  int id = ++_demo_circle_id;
  _demo_circles[id] = new DemoCircle(radius);
  return id;
}

BRIDGE double DemoCircle_area(int handle) {
  if (!_demo_circles.count(handle))
    return 0;
  return _demo_circles[handle]->area();
}
