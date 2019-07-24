// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TX_BLOCK_REWARD_H
#define TX_BLOCK_REWARD_H

#include "tx.h"

class CBlockRewardTx : public CBaseTx {
public:
    uint64_t rewardValue;

public:
    CBlockRewardTx(): CBaseTx(BLOCK_REWARD_TX), rewardValue(0) {}
    CBlockRewardTx(const CBaseTx *pBaseTx) : CBaseTx(BLOCK_REWARD_TX), rewardValue(0) {
        assert(BLOCK_REWARD_TX == pBaseTx->nTxType);
        *this = *(CBlockRewardTx *)pBaseTx;
    }
    CBlockRewardTx(const UnsignedCharArray &accountIn, const uint64_t rewardValueIn, const int32_t nValidHeightIn):
        CBaseTx(BLOCK_REWARD_TX) {
        if (accountIn.size() > 6) {
            txUid = CPubKey(accountIn);
        } else {
            txUid = CRegID(accountIn);
        }
        rewardValue  = rewardValueIn;
        nValidHeight = nValidHeightIn;
    }
    ~CBlockRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(txUid);

        // Do NOT change the order.
        READWRITE(VARINT(rewardValue));
        READWRITE(VARINT(nValidHeight));)

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << txUid << VARINT(rewardValue) << VARINT(nValidHeight);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<CoinType, uint64_t> GetValues() const { return map<CoinType, uint64_t>{{CoinType::WICC, rewardValue}}; }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CBlockRewardTx>(this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state);
};


class CMultiCoinBlockRewardTx : public CBaseTx {
public:
    map<uint8_t /* CoinType */, uint64_t /* reward value */> rewardValues;
    uint64_t profits;  // Profits as delegate according to received votes.

public:
    CMultiCoinBlockRewardTx(): CBaseTx(UCOIN_BLOCK_REWARD_TX), profits(0) {}
    CMultiCoinBlockRewardTx(const CBaseTx *pBaseTx) : CBaseTx(UCOIN_BLOCK_REWARD_TX), profits(0) {
        assert(UCOIN_BLOCK_REWARD_TX == pBaseTx->nTxType);
        *this = *(CMultiCoinBlockRewardTx *)pBaseTx;
    }
    CMultiCoinBlockRewardTx(const CUserID &txUidIn, const map<CoinType, uint64_t> rewardValuesIn,
                            const int32_t validHeightIn)
        : CBaseTx(UCOIN_BLOCK_REWARD_TX) {
        txUid = txUidIn;

        for (const auto &item : rewardValuesIn) {
            rewardValues.emplace(uint8_t(item.first), item.second);
        }

        nValidHeight = validHeightIn;
    }
    ~CMultiCoinBlockRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(rewardValues);
        READWRITE(VARINT(profits));
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << rewardValues << VARINT(profits);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    map<CoinType, uint64_t> GetValues() const;
    uint64_t GetProfits() const { return profits; }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CMultiCoinBlockRewardTx>(this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif // TX_BLOCK_REWARD_H
