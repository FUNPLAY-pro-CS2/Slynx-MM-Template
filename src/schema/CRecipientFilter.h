//
// Created by Michal Přikryl on 17.08.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#pragma once
#include "irecipientfilter.h"

class CRecipientFilter : public IRecipientFilter
{
public:
    CRecipientFilter(NetChannelBufType_t nBufType = BUF_RELIABLE, bool bInitMessage = false) :
        m_nBufType(nBufType), m_bInitMessage(bInitMessage) {}

    CRecipientFilter(const IRecipientFilter* source, CPlayerSlot exceptSlot = -1)
    {
        m_Recipients = CRecipientFilter::GetRecipients();
        m_nBufType = source->GetNetworkBufType();
        m_bInitMessage = source->IsInitMessage();

        if (exceptSlot != -1)
            m_Recipients.Clear(exceptSlot.Get());
    }

    ~CRecipientFilter() override {}

    NetChannelBufType_t GetNetworkBufType(void) const override { return m_nBufType; }
    bool IsInitMessage(void) const override { return m_bInitMessage; }
    const CPlayerBitVec& GetRecipients(void) const override { return m_Recipients; }

    void AddRecipient(CPlayerSlot slot)
    {
        if (slot.Get() >= 0 && slot.Get() < ABSOLUTE_PLAYER_LIMIT)
            m_Recipients.Set(slot.Get());
    }

    int GetRecipientCount() const {
        const uint64 bits = *reinterpret_cast<const uint64*>(&GetRecipients());

        return std::popcount(bits);
    }

protected:
    NetChannelBufType_t m_nBufType;
    bool m_bInitMessage;
    CPlayerBitVec m_Recipients;
};

class CSingleRecipientFilter : public CRecipientFilter
{
public:
    CSingleRecipientFilter(CPlayerSlot nRecipientSlot, NetChannelBufType_t nBufType = BUF_RELIABLE, bool bInitMessage = false)
        : CRecipientFilter(nBufType, bInitMessage)
    {
        if (nRecipientSlot.Get() >= 0 && nRecipientSlot.Get() < ABSOLUTE_PLAYER_LIMIT) m_Recipients.Set(nRecipientSlot.Get());
    }
};