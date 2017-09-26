#ifndef GLOBALPAINTSETTINGS_H
#define GLOBALPAINTSETTINGS_H

class GlobalDrawSettings
{
public:
    inline static bool doDrawDeviants()
    {
        return m_drawDeviants;
    }

    inline static void setDrawDeskewDeviants(bool val)
    {
        if (m_drawDeskewDeviants != val) {
            m_drawDeskewDeviants = val;
            updateParams();
        }
    }

    inline static void setDrawContentDeviants(bool val)
    {
        if (m_drawContentDeviants != val) {
            m_drawContentDeviants = val;
            updateParams();
        }
    }

    inline static void setDrawMarginDeviants(bool val)
    {
        if (m_drawMarginDeviants != val) {
            m_drawMarginDeviants = val;
            updateParams();
        }
    }

    inline static void stageChanged(int i)
    {
        if (m_currentStage != i) {
            m_currentStage = i;
            updateParams();
        }
    }

private:
    inline static void updateParams()
    {
        switch (m_currentStage) {
        case 2:
            m_drawDeviants = m_drawDeskewDeviants;
            break;
        case 3:
            m_drawDeviants = m_drawContentDeviants;
            break;
        case 4:
            m_drawDeviants = m_drawMarginDeviants;
            break;
        default:
            m_drawDeviants = false;
        }
    }

    GlobalDrawSettings() {}

    static bool m_drawDeviants;
    static bool m_drawDeskewDeviants;
    static bool m_drawContentDeviants;
    static bool m_drawMarginDeviants;
    static int m_currentStage;
};

#endif // GLOBALPAINTSETTINGS_H
