#pragma once

#include "../../defines.hpp"

enum eDecorationType {
    DECORATION_NONE = -1,
    DECORATION_GROUPBAR
};

struct SWindowDecorationExtents {
    Vector2D topLeft;
    Vector2D bottomRight;
};

class CWindow;
struct SMonitor;

class IHyprWindowDecoration {
public:
    virtual ~IHyprWindowDecoration() = 0;

    virtual SWindowDecorationExtents getWindowDecorationExtents() = 0;

    virtual void draw(SMonitor*) = 0;

    virtual eDecorationType getDecorationType() = 0;

    virtual void updateWindow(CWindow*) = 0;

    virtual void damageEntire() = 0;
};