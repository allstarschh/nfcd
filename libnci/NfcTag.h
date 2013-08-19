/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *  Tag-reading, tag-writing operations.
 */

#pragma once
#include "SyncEvent.h"
#include "NfcUtil.h"
#include "NativeNfcManager.h"

extern "C"
{
    #include "nfa_rw_api.h"
}

class NativeNfcTag;

class NfcTag
{
public:
    enum ActivationState {Idle, Sleep, Active};
    static const int MAX_NUM_TECHNOLOGY = 10; //max number of technologies supported by one or more tags
    int mTechList [MAX_NUM_TECHNOLOGY]; //array of NFC technologies according to NFC service
    int mTechHandles [MAX_NUM_TECHNOLOGY]; //array of tag handles according to NFC service
    int mTechLibNfcTypes [MAX_NUM_TECHNOLOGY]; //array of detailed tag types according to NFC service
    int mNumTechList; //current number of NFC technologies in the list

    /*******************************************************************************
    **
    ** Function:        NfcTag
    **
    ** Description:     Initialize member variables.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    NfcTag ();


    /*******************************************************************************
    **
    ** Function:        getInstance
    **
    ** Description:     Get a reference to the singleton NfcTag object.
    **
    ** Returns:         Reference to NfcTag object.
    **
    *******************************************************************************/
    static NfcTag& getInstance ();


    /*******************************************************************************
    **
    ** Function:        initialize
    **
    ** Description:     Reset member variables.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void initialize (NativeNfcManager* pNfcManager);


    /*******************************************************************************
    **
    ** Function:        abort
    **
    ** Description:     Unblock all operations.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void abort ();


    /*******************************************************************************
    **
    ** Function:        connectionEventHandler
    **
    ** Description:     Handle connection-related events.
    **                  event: event code.
    **                  data: pointer to event data.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void connectionEventHandler (UINT8 event, tNFA_CONN_EVT_DATA* data);


    /*******************************************************************************
    **
    ** Function:        getActivationState
    **
    ** Description:     What is the current state: Idle, Sleep, or Activated.
    **
    ** Returns:         Idle, Sleep, or Activated.
    **
    *******************************************************************************/
    ActivationState getActivationState ();


    /*******************************************************************************
    **
    ** Function:        setDeactivationState
    **
    ** Description:     Set the current state: Idle or Sleep.
    **                  deactivated: state of deactivation.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    void setDeactivationState (tNFA_DEACTIVATED& deactivated);


    /*******************************************************************************
    **
    ** Function:        setActivationState
    **
    ** Description:     Set the current state to Active.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    void setActivationState ();

    /*******************************************************************************
    **
    ** Function:        getProtocol
    **
    ** Description:     Get the protocol of the current tag.
    **
    ** Returns:         Protocol number.
    **
    *******************************************************************************/
    tNFC_PROTOCOL getProtocol ();


    /*******************************************************************************
    **
    ** Function:        isP2pDiscovered
    **
    ** Description:     Does the peer support P2P?
    **
    ** Returns:         True if the peer supports P2P.
    **
    *******************************************************************************/
    bool isP2pDiscovered ();


    /*******************************************************************************
    **
    ** Function:        selectP2p
    **
    ** Description:     Select the preferred P2P technology if there is a choice.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void selectP2p ();


    /*******************************************************************************
    **
    ** Function:        selectFirstTag
    **
    ** Description:     When multiple tags are discovered, just select the first one to activate.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void selectFirstTag ();


    /*******************************************************************************
    **
    ** Function:        getT1tMaxMessageSize
    **
    ** Description:     Get the maximum size (octet) that a T1T can store.
    **
    ** Returns:         Maximum size in octets.
    **
    *******************************************************************************/
    int getT1tMaxMessageSize ();


    /*******************************************************************************
    **
    ** Function:        isMifareUltralight
    **
    ** Description:     Whether the currently activated tag is Mifare Ultralight.
    **
    ** Returns:         True if tag is Mifare Ultralight.
    **
    *******************************************************************************/
    bool isMifareUltralight ();


    /*******************************************************************************
    **
    ** Function:        isT2tNackResponse
    **
    ** Description:     Whether the response is a T2T NACK response.
    **                  See NFC Digital Protocol Technical Specification (2010-11-17).
    **                  Chapter 9 (Type 2 Tag Platform), section 9.6 (READ).
    **                  response: buffer contains T2T response.
    **                  responseLen: length of the response.
    **
    ** Returns:         True if the response is NACK
    **
    *******************************************************************************/
    bool isT2tNackResponse (const UINT8* response, UINT32 responseLen);

    /*******************************************************************************
    **
    ** Function:        isNdefDetectionTimedOut
    **
    ** Description:     Whether NDEF-detection algorithm has timed out.
    **
    ** Returns:         True if NDEF-detection algorithm timed out.
    **
    *******************************************************************************/
    bool isNdefDetectionTimedOut ();

private:
    ActivationState mActivationState;
    tNFC_PROTOCOL mProtocol;
    int mtT1tMaxMessageSize; //T1T max NDEF message size
    tNFA_STATUS mReadCompletedStatus;
    int mLastKovioUidLen;   // len of uid of last Kovio tag activated
    bool mNdefDetectionTimedOut; // whether NDEF detection algorithm timed out
    tNFC_RF_TECH_PARAMS mTechParams [MAX_NUM_TECHNOLOGY]; //array of technology parameters
    SyncEvent mReadCompleteEvent;
    struct timespec mLastKovioTime; // time of last Kovio tag activation
    UINT8 mLastKovioUid[NFC_KOVIO_MAX_LEN]; // uid of last Kovio tag activated

    NativeNfcManager*     mNfcManager;

    /*******************************************************************************
    **
    ** Function:        IsSameKovio
    **
    ** Description:     Checks if tag activate is the same (UID) Kovio tag previously
    **                  activated.  This is needed due to a problem with some Kovio
    **                  tags re-activating multiple times.
    **                  activationData: data from activation.
    **
    ** Returns:         true if the activation is from the same tag previously
    **                  activated, false otherwise
    **
    *******************************************************************************/
    bool IsSameKovio(tNFA_ACTIVATED& activationData);

    /*******************************************************************************
    **
    ** Function:        discoverTechnologies
    **
    ** Description:     Discover the technologies that NFC service needs by interpreting
    **                  the data strucutures from the stack.
    **                  activationData: data from activation.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void discoverTechnologies (tNFA_ACTIVATED& activationData);


    /*******************************************************************************
    **
    ** Function:        discoverTechnologies
    **
    ** Description:     Discover the technologies that NFC service needs by interpreting
    **                  the data strucutures from the stack.
    **                  discoveryData: data from discovery events(s).
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void discoverTechnologies (tNFA_DISC_RESULT& discoveryData);


    /*******************************************************************************
    **
    ** Function:        createNativeNfcTag
    **
    ** Description:     Create a brand new Java NativeNfcTag object;
    **                  fill the objects's member variables with data;
    **                  notify NFC service;
    **                  activationData: data from activation.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void createNativeNfcTag (tNFA_ACTIVATED& activationData);


    /*******************************************************************************
    **
    ** Function:        fillNativeNfcTagMembers1
    **
    ** Description:     Fill NativeNfcTag's members: mProtocols, mTechList, mTechHandles, mTechLibNfcTypes.
    **                  e: JVM environment.
    **                  tag_cls: Java NativeNfcTag class.
    **                  tag: Java NativeNfcTag object.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void fillNativeNfcTagMembers1 (NativeNfcTag* pNativeNfcTag);


    /*******************************************************************************
    **
    ** Function:        fillNativeNfcTagMembers2
    **
    ** Description:     Fill NativeNfcTag's members: mConnectedTechIndex or mConnectedTechnology.
    **                  The original Google's implementation is in set_target_pollBytes(
    **                  in com_android_nfc_NativeNfcTag.cpp;
    **                  e: JVM environment.
    **                  tag_cls: Java NativeNfcTag class.
    **                  tag: Java NativeNfcTag object.
    **                  activationData: data from activation.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void fillNativeNfcTagMembers2 (NativeNfcTag* pNativeNfcTag);


    /*******************************************************************************
    **
    ** Function:        fillNativeNfcTagMembers3
    **
    ** Description:     Fill NativeNfcTag's members: mTechPollBytes.
    **                  The original Google's implementation is in set_target_pollBytes(
    **                  in com_android_nfc_NativeNfcTag.cpp;
    **                  e: JVM environment.
    **                  tag_cls: Java NativeNfcTag class.
    **                  tag: Java NativeNfcTag object.
    **                  activationData: data from activation.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void fillNativeNfcTagMembers3 (NativeNfcTag* pNativeNfcTag, tNFA_ACTIVATED& activationData);


    /*******************************************************************************
    **
    ** Function:        fillNativeNfcTagMembers4
    **
    ** Description:     Fill NativeNfcTag's members: mTechActBytes.
    **                  The original Google's implementation is in set_target_activationBytes()
    **                  in com_android_nfc_NativeNfcTag.cpp;
    **                  e: JVM environment.
    **                  tag_cls: Java NativeNfcTag class.
    **                  tag: Java NativeNfcTag object.
    **                  activationData: data from activation.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void fillNativeNfcTagMembers4 (NativeNfcTag* pNativeNfcTag, tNFA_ACTIVATED& activationData);


    /*******************************************************************************
    **
    ** Function:        fillNativeNfcTagMembers5
    **
    ** Description:     Fill NativeNfcTag's members: mUid.
    **                  The original Google's implementation is in nfc_jni_Discovery_notification_callback()
    **                  in com_android_nfc_NativeNfcManager.cpp;
    **                  e: JVM environment.
    **                  tag_cls: Java NativeNfcTag class.
    **                  tag: Java NativeNfcTag object.
    **                  activationData: data from activation.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void fillNativeNfcTagMembers5 (NativeNfcTag* pNativeNfcTag, tNFA_ACTIVATED& activationData);


    /*******************************************************************************
    **
    ** Function:        resetTechnologies
    **
    ** Description:     Clear all data related to the technology, protocol of the tag.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void resetTechnologies ();


    /*******************************************************************************
    **
    ** Function:        calculateT1tMaxMessageSize
    **
    ** Description:     Calculate type-1 tag's max message size based on header ROM bytes.
    **                  activate: reference to activation data.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void calculateT1tMaxMessageSize (tNFA_ACTIVATED& activate);
};
