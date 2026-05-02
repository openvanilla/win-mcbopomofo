#pragma once
#include "EditSession.h"
#include "Ipc.h"
#include "McBopomofoTIP.h"

class CStateEditSession : public CEditSessionBase {
public:
    CStateEditSession(ITfContext *pContext, McBopomofoTIP *pTIP, const McBopomofo::IPC::StateUpdatePayload& state);
    ~CStateEditSession();

    STDMETHODIMP DoEditSession(TfEditCookie ec) override;

private:
    McBopomofoTIP *_pTIP;
    McBopomofo::IPC::StateUpdatePayload _state;
};
