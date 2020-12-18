#ifndef CANBERRASOUNDPLAYER_H
#define CANBERRASOUNDPLAYER_H

#include <canberra.h>

class CanberraSoundPlayer
{
public:
    CanberraSoundPlayer();
    ~CanberraSoundPlayer();

    bool isWorking() const { return m_useCanberra; }
    void play();
private:
    static void callback(ca_context *c, unsigned int id, int error_code, void *userdata);
private:
    bool m_useCanberra;
    ca_context *m_canberraContext;
    ca_proplist *m_canberraProperties;
};

#endif // CANBERRASOUNDPLAYER_H
