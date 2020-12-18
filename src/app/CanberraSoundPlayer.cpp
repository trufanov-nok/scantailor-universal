#include "CanberraSoundPlayer.h"
#include "version.h"
#include <QApplication>

CanberraSoundPlayer::CanberraSoundPlayer():
    m_useCanberra(false),
    m_canberraContext(nullptr),
    m_canberraProperties(nullptr)
{
    if (ca_context_create(&m_canberraContext) != CA_SUCCESS) {
        return;
    }

    if (ca_context_change_props(m_canberraContext,
                                CA_PROP_APPLICATION_NAME, qApp->applicationDisplayName().toStdString().c_str(),
                                CA_PROP_APPLICATION_ID, "org.scantailor.universal",
                                CA_PROP_APPLICATION_VERSION, VERSION,
                                CA_PROP_APPLICATION_ICON_NAME, "scantailor-universal", nullptr) != CA_SUCCESS) {
        return;
    }

    ca_proplist_create(&m_canberraProperties);
    ca_proplist_sets(m_canberraProperties, CA_PROP_EVENT_ID, "complete");
    ca_proplist_sets(m_canberraProperties, CA_PROP_EVENT_DESCRIPTION, QString(QObject::tr("Pages processing is complete.")).toStdString().c_str());
    m_useCanberra = true;
}


void
CanberraSoundPlayer::callback(ca_context *c, unsigned int id, int error_code, void *userdata)
{
    Q_UNUSED(c);
    Q_UNUSED(id);
    CanberraSoundPlayer* pleer = static_cast<CanberraSoundPlayer*>(userdata);
    if (pleer) {
        pleer->m_useCanberra = (error_code == CA_SUCCESS);
    }
}

void
CanberraSoundPlayer::play()
{
    if (!m_useCanberra) {
        return;
    }

    ca_context_play_full(m_canberraContext, 0, m_canberraProperties, &CanberraSoundPlayer::callback, this);
}


CanberraSoundPlayer::~CanberraSoundPlayer()
{
    ca_context_destroy(m_canberraContext);
    ca_proplist_destroy(m_canberraProperties);
}
