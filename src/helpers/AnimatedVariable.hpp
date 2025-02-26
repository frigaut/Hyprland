#pragma once

#include "../defines.hpp"
#include <any>

enum ANIMATEDVARTYPE {
    AVARTYPE_INVALID = -1,
    AVARTYPE_FLOAT,
    AVARTYPE_VECTOR,
    AVARTYPE_COLOR
};

enum AVARDAMAGEPOLICY {
    AVARDAMAGE_INVALID = -1,
    AVARDAMAGE_ENTIRE = 0,
    AVARDAMAGE_BORDER
};

class CAnimationManager;
class CWorkspace;
struct SLayerSurface;

class CAnimatedVariable {
public:
    CAnimatedVariable(); // dummy var

    void create(ANIMATEDVARTYPE, float* speed, int64_t* enabled, std::string* pBezier, void* pWindow, AVARDAMAGEPOLICY);
    void create(ANIMATEDVARTYPE, std::any val, float* speed, int64_t* enabled, std::string* pBezier, void* pWindow, AVARDAMAGEPOLICY);

    ~CAnimatedVariable();

    void unregister();

    // gets the current vector value (real time)
    const Vector2D& vec() const {
        RASSERT(m_eVarType == AVARTYPE_VECTOR, "Tried to access vec() of AVARTYPE %i!", m_eVarType);
        return m_vValue;
    }

    // gets the current float value (real time)
    const float& fl() const {
        RASSERT(m_eVarType == AVARTYPE_FLOAT, "Tried to access fl() of AVARTYPE %i!", m_eVarType);
        return m_fValue;
    }

    // gets the current color value (real time)
    const CColor& col() const {
        RASSERT(m_eVarType == AVARTYPE_COLOR, "Tried to access col() of AVARTYPE %i!", m_eVarType);
        return m_cValue;
    }

    // gets the goal vector value
    const Vector2D& goalv() const {
        RASSERT(m_eVarType == AVARTYPE_VECTOR, "Tried to access goalv() of AVARTYPE %i!", m_eVarType);
        return m_vGoal;
    }

    // gets the goal float value
    const float& goalf() const {
        RASSERT(m_eVarType == AVARTYPE_FLOAT, "Tried to access goalf() of AVARTYPE %i!", m_eVarType);
        return m_fGoal;
    }

    // gets the goal color value
    const CColor& goalc() const {
        RASSERT(m_eVarType == AVARTYPE_COLOR, "Tried to access goalc() of AVARTYPE %i!", m_eVarType);
        return m_cGoal;
    }

    void operator=(const Vector2D& v) {
        RASSERT(m_eVarType == AVARTYPE_VECTOR, "Tried to access =v of AVARTYPE %i!", m_eVarType);
        m_vGoal = v;
        animationBegin = std::chrono::system_clock::now();
        m_vBegun = m_vValue;
    }

    void operator=(const float& v) {
        RASSERT(m_eVarType == AVARTYPE_FLOAT, "Tried to access =f of AVARTYPE %i!", m_eVarType);
        m_fGoal = v;
        animationBegin = std::chrono::system_clock::now();
        m_fBegun = m_fValue;
    }

    void operator=(const CColor& v) {
        RASSERT(m_eVarType == AVARTYPE_COLOR, "Tried to access =c of AVARTYPE %i!", m_eVarType);
        m_cGoal = v;
        animationBegin = std::chrono::system_clock::now();
        m_cBegun = m_cValue;
    }

    // Sets the actual stored value, without affecting the goal, but resets the timer
    void setValue(const Vector2D& v) {
        RASSERT(m_eVarType == AVARTYPE_VECTOR, "Tried to access setValue(v) of AVARTYPE %i!", m_eVarType);
        m_vValue = v;
        animationBegin = std::chrono::system_clock::now();
        m_vBegun = m_vValue;
    }

    // Sets the actual stored value, without affecting the goal, but resets the timer
    void setValue(const float& v) {
        RASSERT(m_eVarType == AVARTYPE_FLOAT, "Tried to access setValue(f) of AVARTYPE %i!", m_eVarType);
        m_fValue = v;
        animationBegin = std::chrono::system_clock::now();
        m_vBegun = m_vValue;
    }

    // Sets the actual stored value, without affecting the goal, but resets the timer
    void setValue(const CColor& v) {
        RASSERT(m_eVarType == AVARTYPE_COLOR, "Tried to access setValue(c) of AVARTYPE %i!", m_eVarType);
        m_cValue = v;
        animationBegin = std::chrono::system_clock::now();
        m_vBegun = m_vValue;
    }

    // Sets the actual value and goal
    void setValueAndWarp(const Vector2D& v) {
        RASSERT(m_eVarType == AVARTYPE_VECTOR, "Tried to access setValueAndWarp(v) of AVARTYPE %i!", m_eVarType);
        m_vGoal = v;
        warp();
    }

    // Sets the actual value and goal
    void setValueAndWarp(const float& v) {
        RASSERT(m_eVarType == AVARTYPE_FLOAT, "Tried to access setValueAndWarp(f) of AVARTYPE %i!", m_eVarType);
        m_fGoal = v;
        warp();
    }

    // Sets the actual value and goal
    void setValueAndWarp(const CColor& v) {
        RASSERT(m_eVarType == AVARTYPE_COLOR, "Tried to access setValueAndWarp(c) of AVARTYPE %i!", m_eVarType);
        m_cGoal = v;
        warp();
    }

    // checks if an animation is in progress
    bool isBeingAnimated() {
        switch (m_eVarType) {
            case AVARTYPE_FLOAT:
                return m_fValue != m_fGoal;
            case AVARTYPE_VECTOR: 
                return m_vValue != m_vGoal;
            case AVARTYPE_COLOR:
                return m_cValue != m_cGoal;
            default:
                return false;
        }

        return false; // unreachable
    }

    void warp() {
        switch (m_eVarType) {
            case AVARTYPE_FLOAT: {
                m_fValue = m_fGoal;
                break;
            }
            case AVARTYPE_VECTOR: {
                m_vValue = m_vGoal;
                break;
            }
            case AVARTYPE_COLOR: {
                m_cValue = m_cGoal;
                break;
            }
            default:
                break;
        }
    }

private:

    Vector2D        m_vValue = Vector2D(0,0);
    float           m_fValue = 0;
    CColor          m_cValue;

    Vector2D        m_vGoal = Vector2D(0,0);
    float           m_fGoal = 0;
    CColor          m_cGoal;

    Vector2D        m_vBegun = Vector2D(0,0);
    float           m_fBegun = 0;
    CColor          m_cBegun;

    float*          m_pSpeed = nullptr;
    int64_t*        m_pEnabled = nullptr;

    // owners
    void*           m_pWindow = nullptr;
    void*           m_pWorkspace = nullptr;
    void*           m_pLayer = nullptr;

    std::string*    m_pBezier = nullptr;

    bool            m_bDummy = true;

    std::chrono::system_clock::time_point animationBegin;

    ANIMATEDVARTYPE     m_eVarType      = AVARTYPE_INVALID;
    AVARDAMAGEPOLICY    m_eDamagePolicy = AVARDAMAGE_INVALID;

    friend class CAnimationManager;
    friend class CWorkspace;
    friend struct SLayerSurface;
};