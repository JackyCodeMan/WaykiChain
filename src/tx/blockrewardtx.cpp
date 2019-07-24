// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "blockrewardtx.h"

#include "main.h"

bool CBlockRewardTx::CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state) {
    return true;
}

bool CBlockRewardTx::ExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CBlockRewardTx::ExecuteTx, read source addr %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (0 == nIndex) {
        // When the reward transaction is immature, should NOT update account's balances.
    } else if (-1 == nIndex) {
        // When the reward transaction is mature, update account's balances, i.e, assign the reward value to
        // the target account.
        account.OperateBalance("WICC", ADD_FREE, rewardValue);

    } else {
        return ERRORMSG("CBlockRewardTx::ExecuteTx, invalid index");
    }

    if (!cw.accountCache.SetAccount(CUserID(account.keyid), account))
        return state.DoS(100, ERRORMSG("CBlockRewardTx::ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    // Block reward transaction will execute twice, but need to save once when index equals to zero.
    if (nIndex == 0 && !SaveTxAddresses(nHeight, nIndex, cw, state, {txUid}))
        return false;

    return true;
}

string CBlockRewardTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf("txType=%s, hash=%s, ver=%d, account=%s, keyId=%s, rewardValue=%ld\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, txUid.ToString(), keyId.GetHex(), rewardValue);
}

Object CBlockRewardTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    result.push_back(Pair("txid",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("uid",            txUid.ToString()));
    result.push_back(Pair("addr",           keyId.ToAddress()));
    result.push_back(Pair("reward_value",   rewardValue));
    result.push_back(Pair("valid_height",   nValidHeight));

    return result;
}

bool CBlockRewardTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (txUid.type() == typeid(CRegID)) {
        if (!cw.accountCache.GetKeyId(txUid, keyId))
            return false;

        keyIds.insert(keyId);
    } else if (txUid.type() == typeid(CPubKey)) {
        CPubKey pubKey = txUid.get<CPubKey>();
        if (!pubKey.IsFullyValid())
            return false;

        keyIds.insert(pubKey.GetKeyId());
    }

    return true;
}

bool CMultiCoinBlockRewardTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    return true;
}

bool CMultiCoinBlockRewardTx::ExecuteTx(int32_t height, int32_t nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CMultiCoinBlockRewardTx::ExecuteTx, read source addr %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (0 == nIndex) {
        // When the reward transaction is immature, should NOT update account's balances.
    } else if (-1 == nIndex) {
        // When the reward transaction is mature, update account's balances, i.e, assgin the reward values to
        // the target account.
        for (const auto &item : rewardValues) {
            switch (item.first/* CoinType */) {
                case CoinType::WICC: account.GetToken("WICC").free_amount += item.second; break;
                case CoinType::WUSD: account.free_scoins += item.second; break;
                case CoinType::WGRT: account.free_fcoins += item.second; break;
                default: return ERRORMSG("CMultiCoinBlockRewardTx::ExecuteTx, invalid coin type");
            }
        }

        // Assign profits to the delegate's account.
        account.GetToken("WICC").free_amount += profits;
    } else {
        return ERRORMSG("CMultiCoinBlockRewardTx::ExecuteTx, invalid index");
    }

    if (!cw.accountCache.SetAccount(CUserID(account.keyid), account))
        return state.DoS(100, ERRORMSG("CMultiCoinBlockRewardTx::ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    // Block reward transaction will execute twice, but need to save once when index equals to zero.
    if (nIndex == 0 && !SaveTxAddresses(height, nIndex, cw, state, {txUid}))
        return false;

    return true;
}

map<CoinType, uint64_t> CMultiCoinBlockRewardTx::GetValues() const {
    map<CoinType, uint64_t> rewardValuesOut;
    for (const auto &item : rewardValues) {
        rewardValuesOut.emplace(CoinType(item.first), item.second);
    }

    return rewardValuesOut;
}

string CMultiCoinBlockRewardTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string rewardValue;
    for (const auto &item : rewardValues) {
        rewardValue += strprintf("%s: %lu, ", GetCoinTypeName(CoinType(item.first)), item.second);
    }

    return strprintf("txType=%s, hash=%s, ver=%d, account=%s, addr=%s, rewardValue=%s, nValidHeight=%d\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(), rewardValue, nValidHeight);
}

Object CMultiCoinBlockRewardTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    Object rewardValue;
    for (const auto &item : rewardValues) {
        rewardValue.push_back(Pair(GetCoinTypeName(CoinType(item.first)), item.second));
    }

    result.push_back(Pair("txid",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("uid",            txUid.ToString()));
    result.push_back(Pair("addr",           keyId.ToAddress()));
    result.push_back(Pair("reward_value",   rewardValue));
    result.push_back(Pair("valid_height",   nValidHeight));

    return result;
}

bool CMultiCoinBlockRewardTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (!cw.accountCache.GetKeyId(txUid, keyId))
        return false;

    keyIds.insert(keyId);

    return true;
}