#include "AnimationManager.hpp"
#include "../Compositor.hpp"

CAnimationManager::CAnimationManager() {
    std::vector<Vector2D> points = {Vector2D(0, 0.75f), Vector2D(0.15f, 1.f)};
    m_mBezierCurves["default"].setup(&points);
}

void CAnimationManager::removeAllBeziers() {
    m_mBezierCurves.clear();

    // add the default one
    std::vector<Vector2D> points = {Vector2D(0, 0.75f), Vector2D(0.15f, 1.f)};
    m_mBezierCurves["default"].setup(&points);
}

void CAnimationManager::addBezierWithName(std::string name, const Vector2D& p1, const Vector2D& p2) {
    std::vector points = {p1, p2};
    m_mBezierCurves[name].setup(&points);
}

void CAnimationManager::tick() {

    bool animationsDisabled = false;

    static auto *const PANIMENABLED = &g_pConfigManager->getConfigValuePtr("animations:enabled")->intValue;

    if (!*PANIMENABLED)
        animationsDisabled = true;

    static auto *const  PANIMSPEED        = &g_pConfigManager->getConfigValuePtr("animations:speed")->floatValue;
    static auto *const  PBORDERSIZE       = &g_pConfigManager->getConfigValuePtr("general:border_size")->intValue;
    static auto *const  BEZIERSTR         = &g_pConfigManager->getConfigValuePtr("animations:curve")->strValue;

    auto DEFAULTBEZIER = m_mBezierCurves.find(*BEZIERSTR);
    if (DEFAULTBEZIER == m_mBezierCurves.end())
        DEFAULTBEZIER = m_mBezierCurves.find("default");

    for (auto& av : m_lAnimatedVariables) {
        // get speed
        const auto SPEED = *av->m_pSpeed == 0 ? *PANIMSPEED : *av->m_pSpeed;

        // window stuff
        const auto PWINDOW = (CWindow*)av->m_pWindow;
        const auto PWORKSPACE = (CWorkspace*)av->m_pWorkspace;
        const auto PLAYER = (SLayerSurface*)av->m_pLayer;

        wlr_box WLRBOXPREV = {0,0,0,0};
        if (PWINDOW) {
            WLRBOXPREV = {(int)PWINDOW->m_vRealPosition.vec().x - (int)*PBORDERSIZE - 1, (int)PWINDOW->m_vRealPosition.vec().y - (int)*PBORDERSIZE - 1, (int)PWINDOW->m_vRealSize.vec().x + 2 * (int)*PBORDERSIZE + 2, (int)PWINDOW->m_vRealSize.vec().y + 2 * (int)*PBORDERSIZE + 2};
        } else if (PWORKSPACE) {
            const auto PMONITOR = g_pCompositor->getMonitorFromID(PWORKSPACE->m_iMonitorID);
            WLRBOXPREV = {(int)PMONITOR->vecPosition.x, (int)PMONITOR->vecPosition.y, (int)PMONITOR->vecSize.x, (int)PMONITOR->vecSize.y};
        } else if (PLAYER) {
            WLRBOXPREV = PLAYER->geometry;
        }

        // beziers are with a switch unforto
        // TODO: maybe do something cleaner

        // get the spent % (0 - 1)
        const auto DURATIONPASSED = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - av->animationBegin).count();
        const float SPENT = std::clamp((DURATIONPASSED / 100.f) / SPEED, 0.f, 1.f);

        switch (av->m_eVarType) {
            case AVARTYPE_FLOAT: {
                if (!deltazero(av->m_fValue, av->m_fGoal)) {

                    // for disabled anims just warp
                    if (av->m_pEnabled == 0 || animationsDisabled) {
                        av->warp();
                        break;
                    }

                    const auto DELTA = av->m_fGoal - av->m_fBegun;
                    const auto BEZIER = m_mBezierCurves.find(*av->m_pBezier);

                    if (BEZIER != m_mBezierCurves.end())
                        av->m_fValue = av->m_fBegun + BEZIER->second.getYForPoint(SPENT) * DELTA;
                    else
                        av->m_fValue = av->m_fBegun + DEFAULTBEZIER->second.getYForPoint(SPENT) * DELTA;

                    if (SPENT >= 1.f) {
                        av->warp();
                    }
                } else {
                    continue; // dont process
                }
                break;
            }
            case AVARTYPE_VECTOR: {
                if (!deltazero(av->m_vValue, av->m_vGoal)) {

                    // for disabled anims just warp
                    if (av->m_pEnabled == 0 || animationsDisabled) {
                        av->warp();
                        break;
                    }

                    const auto DELTA = av->m_vGoal - av->m_vBegun;
                    const auto BEZIER = m_mBezierCurves.find(*av->m_pBezier);

                    if (BEZIER != m_mBezierCurves.end())
                        av->m_vValue = av->m_vBegun + DELTA * BEZIER->second.getYForPoint(SPENT);
                    else
                        av->m_vValue = av->m_vBegun + DELTA * DEFAULTBEZIER->second.getYForPoint(SPENT);

                    if (SPENT >= 1.f) {
                        av->warp();
                    }
                } else {
                    continue;  // dont process
                }
                break;
            }
            case AVARTYPE_COLOR: {
                if (!deltazero(av->m_cValue, av->m_cGoal)) {

                    // for disabled anims just warp
                    if (av->m_pEnabled == 0 || animationsDisabled) {
                        av->warp();
                        break;
                    }

                    const auto DELTA = av->m_cGoal - av->m_cBegun;
                    const auto BEZIER = m_mBezierCurves.find(*av->m_pBezier);

                    if (BEZIER != m_mBezierCurves.end())
                        av->m_cValue = av->m_cBegun + DELTA * BEZIER->second.getYForPoint(SPENT);
                    else
                        av->m_cValue = av->m_cBegun + DELTA * DEFAULTBEZIER->second.getYForPoint(SPENT);

                    if (SPENT >= 1.f) {
                        av->warp();
                    }
                } else {
                    continue;  // dont process
                }
                break;
            }
            default: {
                ;
            }
        }

        // damage the window with the damage policy
        switch (av->m_eDamagePolicy) {
            case AVARDAMAGE_ENTIRE: {
                g_pHyprRenderer->damageBox(&WLRBOXPREV);

                if (PWINDOW) {
                    g_pHyprRenderer->damageWindow(PWINDOW);
                    for (auto& wd : PWINDOW->m_dWindowDecorations) {
                        wd->updateWindow(PWINDOW);
                    }
                }
                break;
            }
            case AVARDAMAGE_BORDER: {
                RASSERT(PWINDOW, "Tried to AVARDAMAGE_BORDER a non-window AVAR!");
                
                // damage only the border.
                static auto *const PROUNDING = &g_pConfigManager->getConfigValuePtr("decoration:rounding")->intValue;
                const auto ROUNDINGSIZE = *PROUNDING + 1;
                const auto BORDERSIZE = *PBORDERSIZE;

                // damage for old box
                g_pHyprRenderer->damageBox(WLRBOXPREV.x - BORDERSIZE, WLRBOXPREV.y - BORDERSIZE, WLRBOXPREV.width + 2 * BORDERSIZE, BORDERSIZE + ROUNDINGSIZE);                              // top
                g_pHyprRenderer->damageBox(WLRBOXPREV.x - BORDERSIZE, WLRBOXPREV.y - BORDERSIZE, BORDERSIZE + ROUNDINGSIZE, WLRBOXPREV.height + 2 * BORDERSIZE);                             // left
                g_pHyprRenderer->damageBox(WLRBOXPREV.x + WLRBOXPREV.width - ROUNDINGSIZE, WLRBOXPREV.y - BORDERSIZE, BORDERSIZE + ROUNDINGSIZE, WLRBOXPREV.height + 2 * BORDERSIZE);        // right
                g_pHyprRenderer->damageBox(WLRBOXPREV.x, WLRBOXPREV.y + WLRBOXPREV.height - ROUNDINGSIZE, WLRBOXPREV.width + 2 * BORDERSIZE, BORDERSIZE + ROUNDINGSIZE);                     // bottom

                // damage for new box
                const wlr_box WLRBOXNEW = {PWINDOW->m_vRealPosition.vec().x, PWINDOW->m_vRealPosition.vec().y, PWINDOW->m_vRealSize.vec().x, PWINDOW->m_vRealSize.vec().y};
                g_pHyprRenderer->damageBox(WLRBOXNEW.x - BORDERSIZE, WLRBOXNEW.y - BORDERSIZE, WLRBOXNEW.width + 2 * BORDERSIZE, BORDERSIZE + ROUNDINGSIZE);                                // top
                g_pHyprRenderer->damageBox(WLRBOXNEW.x - BORDERSIZE, WLRBOXNEW.y - BORDERSIZE, BORDERSIZE + ROUNDINGSIZE, WLRBOXNEW.height + 2 * BORDERSIZE);                               // left
                g_pHyprRenderer->damageBox(WLRBOXNEW.x + WLRBOXNEW.width - ROUNDINGSIZE, WLRBOXNEW.y - BORDERSIZE, BORDERSIZE + ROUNDINGSIZE, WLRBOXNEW.height + 2 * BORDERSIZE);           // right
                g_pHyprRenderer->damageBox(WLRBOXNEW.x, WLRBOXNEW.y + WLRBOXNEW.height - ROUNDINGSIZE, WLRBOXNEW.width + 2 * BORDERSIZE, BORDERSIZE + ROUNDINGSIZE);                        // bottom

                break;
            }
            default: {
                Debug::log(ERR, "av has damage policy INVALID???");
                break;
            }
        }
        

        // set size and pos if valid, but only if damage policy entire (dont if border for example)
        if (g_pCompositor->windowValidMapped(PWINDOW) && av->m_eDamagePolicy == AVARDAMAGE_ENTIRE)
            g_pXWaylandManager->setWindowSize(PWINDOW, PWINDOW->m_vRealSize.goalv());
    }
}

bool CAnimationManager::deltaSmallToFlip(const Vector2D& a, const Vector2D& b) {
    return std::abs(a.x - b.x) < 0.5f && std::abs(a.y - b.y) < 0.5f;
}

bool CAnimationManager::deltaSmallToFlip(const CColor& a, const CColor& b) {
    return std::abs(a.r - b.r) < 0.5f && std::abs(a.g - b.g) < 0.5f && std::abs(a.b - b.b) < 0.5f && std::abs(a.a - b.a) < 0.5f;
}

bool CAnimationManager::deltaSmallToFlip(const float& a, const float& b) {
    return std::abs(a - b) < 0.5f;
}

bool CAnimationManager::deltazero(const Vector2D& a, const Vector2D& b) {
    return a.x == b.x && a.y == b.y;
}

bool CAnimationManager::deltazero(const float& a, const float& b) {
    return a == b;
}

bool CAnimationManager::deltazero(const CColor& a, const CColor& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

//
// Anims
//
//

void CAnimationManager::animationPopin(CWindow* pWindow, bool close) {
    const auto GOALPOS = pWindow->m_vRealPosition.goalv();
    const auto GOALSIZE = pWindow->m_vRealSize.goalv();

    if (!close) {
        pWindow->m_vRealPosition.setValue(GOALPOS + GOALSIZE / 2.f);
        pWindow->m_vRealSize.setValue(Vector2D(5, 5));
    } else {
        pWindow->m_vRealPosition = GOALPOS + GOALSIZE / 2.f;
        pWindow->m_vRealSize = Vector2D(5, 5);
    }
}

void CAnimationManager::animationSlide(CWindow* pWindow, std::string force, bool close) {
    pWindow->m_vRealSize.warp();  // size we preserve in slide

    const auto GOALPOS = pWindow->m_vRealPosition.goalv();
    const auto GOALSIZE = pWindow->m_vRealSize.goalv();

    const auto PMONITOR = g_pCompositor->getMonitorFromID(pWindow->m_iMonitorID);

    Vector2D posOffset;

    if (force != "") {
        if (force == "bottom") posOffset = Vector2D(GOALPOS.x, PMONITOR->vecPosition.y + PMONITOR->vecSize.y);
        else if (force == "left") posOffset = GOALPOS - Vector2D(GOALSIZE.x, 0);
        else if (force == "right") posOffset = GOALPOS + Vector2D(GOALSIZE.x, 0);
        else posOffset = Vector2D(GOALPOS.x, PMONITOR->vecPosition.y - GOALSIZE.y);

        if (!close)
            pWindow->m_vRealPosition.setValue(posOffset);
        else
            pWindow->m_vRealPosition = posOffset;

        return;
    }

    const auto MIDPOINT = GOALPOS + GOALSIZE / 2.f;

    // check sides it touches
    const bool DISPLAYLEFT = STICKS(pWindow->m_vPosition.x, PMONITOR->vecPosition.x + PMONITOR->vecReservedTopLeft.x);
    const bool DISPLAYRIGHT = STICKS(pWindow->m_vPosition.x + pWindow->m_vSize.x, PMONITOR->vecPosition.x + PMONITOR->vecSize.x - PMONITOR->vecReservedBottomRight.x);
    const bool DISPLAYTOP = STICKS(pWindow->m_vPosition.y, PMONITOR->vecPosition.y + PMONITOR->vecReservedTopLeft.y);
    const bool DISPLAYBOTTOM = STICKS(pWindow->m_vPosition.y + pWindow->m_vSize.y, PMONITOR->vecPosition.y + PMONITOR->vecSize.y - PMONITOR->vecReservedBottomRight.y);

    if (DISPLAYBOTTOM && DISPLAYTOP) {
        if (DISPLAYLEFT && DISPLAYRIGHT) {
            posOffset = GOALPOS + Vector2D(0, GOALSIZE.y);
        } else if (DISPLAYLEFT) {
            posOffset = GOALPOS - Vector2D(GOALSIZE.x, 0);
        } else {
            posOffset = GOALPOS + Vector2D(GOALSIZE.x, 0);
        }
    } else if (DISPLAYTOP) {
        posOffset = GOALPOS - Vector2D(0, GOALSIZE.y);
    } else if (DISPLAYBOTTOM) {
        posOffset = GOALPOS + Vector2D(0, GOALSIZE.y);
    } else {
        if (MIDPOINT.y > PMONITOR->vecPosition.y + PMONITOR->vecSize.y / 2.f)
            posOffset = Vector2D(GOALPOS.x, PMONITOR->vecPosition.y + PMONITOR->vecSize.y);
        else
            posOffset = Vector2D(GOALPOS.x, PMONITOR->vecPosition.y - GOALSIZE.y);
    }

    if (!close)
        pWindow->m_vRealPosition.setValue(posOffset);
    else
        pWindow->m_vRealPosition = posOffset;
}

void CAnimationManager::onWindowPostCreateClose(CWindow* pWindow, bool close) {
    auto ANIMSTYLE = g_pConfigManager->getString("animations:windows_style");
    transform(ANIMSTYLE.begin(), ANIMSTYLE.end(), ANIMSTYLE.begin(), ::tolower);

    // if the window is not being animated, that means the layout set a fixed size for it, don't animate.
    if (!pWindow->m_vRealPosition.isBeingAnimated() && !pWindow->m_vRealSize.isBeingAnimated())
        return;

    if (pWindow->m_sAdditionalConfigData.animationStyle != "") {
        // the window has config'd special anim
        if (pWindow->m_sAdditionalConfigData.animationStyle.find("slide") == 0) {
            if (pWindow->m_sAdditionalConfigData.animationStyle.find(' ') != std::string::npos) {
                // has a direction
                animationSlide(pWindow, pWindow->m_sAdditionalConfigData.animationStyle.substr(pWindow->m_sAdditionalConfigData.animationStyle.find(' ') + 1), close);
            } else {
                animationSlide(pWindow, "", close);
            }
        } else {
            // anim popin, fallback
            animationPopin(pWindow, close);
        }
    } else {
        if (ANIMSTYLE == "slide") {
            animationSlide(pWindow, "", close);
        } else {
            // anim popin, fallback
            animationPopin(pWindow, close);
        }
    }
}