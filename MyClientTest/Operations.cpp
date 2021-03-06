#include "stdafx.h" 
#include "Operations.h"


COperations::COperations(CInput* poInput, COutput* poOutput, CRTProtocol* poRTProtocol)
{
    mpoInput      = poInput;
    mpoOutput     = poOutput;
    mpoRTProtocol = poRTProtocol;
}


void COperations::DiscoverRTServers(char* tServerAddr, int nServerAddrLen, unsigned short* pnPort)
{
    char           pMessage[256];
    unsigned int   nAddr;
    unsigned short nBasePort;

    if (mpoRTProtocol->DiscoverRTServer(0, false)) // Use random UDP server port.
    {
        unsigned int nResponsCount = mpoRTProtocol->GetNumberOfDiscoverResponses();

        if (nResponsCount == 0)
        {
            printf("No QTM RT Servers found.\n\n");
            return;
        }

        printf("QTM RT Servers found:\n\n");

        for (unsigned int nResponse = 0; nResponse < nResponsCount; nResponse++)
        {
            if (mpoRTProtocol->GetDiscoverResponse(nResponse, nAddr, nBasePort, pMessage, sizeof(pMessage)))
            {
                if (nBasePort > 0)
                {
                    char tAddrStr[32];
                    sprintf_s(tAddrStr, sizeof(tAddrStr), "%d.%d.%d.%d", 0xff & nAddr, 0xff & (nAddr >> 8), 0xff & (nAddr >> 16), 0xff & (nAddr >> 24));
                    if (tServerAddr != NULL)
                    {
                        printf("%-2d : ", nResponse + 1);
                    }
                    printf("%-15s %d - %s\n", tAddrStr, nBasePort, pMessage);
                }
                else
                {
                    printf("%d.%d.%d.%d\t- %s\n", 0xff & nAddr, 0xff & (nAddr >> 8), 0xff & (nAddr >> 16),
                        0xff & (nAddr >> 24), pMessage);
                }
            }
            else
            {
                break;
            }
        };
    }
    if (tServerAddr != NULL)
    {
        printf("\nSelect QTM RT Server to connect to (1 - %d): ", mpoRTProtocol->GetNumberOfDiscoverResponses());
        int nSelection = mpoInput->ReadInt(0);
        if (mpoRTProtocol->GetDiscoverResponse(nSelection - 1, nAddr, *pnPort, pMessage, sizeof(pMessage)))
        {
            sprintf_s(tServerAddr, nServerAddrLen, "%d.%d.%d.%d", 0xff & nAddr, 0xff & (nAddr >> 8), 0xff & (nAddr >> 16), 0xff & (nAddr >> 24));
        }
        else
        {
            tServerAddr[0] = 0;
        }
    }

    printf("\n");

} // DiscoverRTServers


void COperations::MonitorEvents()
{
    CRTPacket::EEvent       eEvent;
    CRTPacket::EPacketType  ePacketType;
    unsigned int            nEventCount = 0;

    printf("\n\nWaiting for QTM Events. Abort with any key.\n\n");

    while (mpoInput->CheckKeyPressed() == false)
    {
        int nRecv = mpoRTProtocol->ReceiveRTPacket(ePacketType, false);

        if (nRecv == -1 || ePacketType == CRTPacket::PacketError)
        {
            break;
        }
        if (nRecv > 0 && ePacketType == CRTPacket::PacketEvent)
        {
            printf("#%d ", nEventCount++);

            if (mpoRTProtocol->GetRTPacket()->GetEvent(eEvent))
            {
                mpoOutput->PrintEvent(eEvent);
            }
        }
    }
} // MonitorEvents


void COperations::ViewSettings()
{
    if (mpoRTProtocol->ReadCameraSystemSettings() == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read general settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else
    {
        mpoOutput->PrintGeneralSettings(mpoRTProtocol);
    }

    bool bDataAvailable;

    if (mpoRTProtocol->Read3DSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read 3D settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->Print3DSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->Read6DOFSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read 6DOF settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->Print6DOFSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->ReadGazeVectorSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read Gaze vector settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->PrintGazeVectorSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->ReadAnalogSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read Analog settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->PrintAnalogSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->ReadForceSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read Force settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->PrintForceSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->ReadImageSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read Image settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->PrintImageSettings(mpoRTProtocol);
    }
} // ViewSettings


void COperations::ChangeSettings(CInput::EOperation eOperation)
{
    unsigned int              nCameraId;
    bool                      bEnable;
    int                       nImageFormat;
    unsigned int              nWidth;
    unsigned int              nHeight;
    float                     fLeftCrop;
    float                     fTopCrop;
    float                     fRightCrop;
    float                     fBottomCrop;
    char                      pPassword[31];
    bool                      bGotControl;

    pPassword[0] = 0;
    bGotControl  = false;

    do 
    {
        // Take control over QTM
        if (mpoRTProtocol->TakeControl(pPassword))
        {
            bGotControl = true;

            if (eOperation == CInput::ChangeGeneralSystemSettings)
            {
                printf("\n\nInput General Settings\n\n");

                unsigned int nCaptureFrequency;
                float fCaptureTime;
                bool bExternalTrigger;
                bool startOnTrigNO;
                bool startOnTrigNC;
                bool startOnTrigSoftware;

                mpoInput->ReadSystemSettings(nCaptureFrequency, fCaptureTime, bExternalTrigger, startOnTrigNO, startOnTrigNC, startOnTrigSoftware);

                if (mpoRTProtocol->SetSystemSettings(&nCaptureFrequency, &fCaptureTime, &bExternalTrigger, &startOnTrigNO, &startOnTrigNC, &startOnTrigSoftware, NULL, NULL, NULL))
                {
                    printf("Change General Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change General Settings Failed\n\n");
                }
            }

            if (eOperation == CInput::ChangeExtTimebaseSettings)
            {
                bool         bEnabled;
                int          nSignalSource;
                bool         bSignalModePeriodic;
                unsigned int nMultiplier;
                unsigned int nDivisor;
                unsigned int nFrequencyTolerance;
                float        fNominalFrequency;
                bool         bNegativeEdge;
                unsigned int nSignalShutterDelay;
                float        fNonPeriodicTimeout;

                printf("\n\nInput External Time Base Settings\n\n");

                mpoInput->ReadExtTimeBaseSettings(bEnabled,          nSignalSource, bSignalModePeriodic,
                                                         nMultiplier,       nDivisor,      nFrequencyTolerance,
                                                         fNominalFrequency, bNegativeEdge, nSignalShutterDelay,
                                                         fNonPeriodicTimeout);

                if (mpoRTProtocol->SetExtTimeBaseSettings(&bEnabled,            (CRTProtocol::ESignalSource*)&nSignalSource,
                                                         &bSignalModePeriodic, &nMultiplier,       &nDivisor,      
                                                         &nFrequencyTolerance, &fNominalFrequency, &bNegativeEdge,
                                                         &nSignalShutterDelay, &fNonPeriodicTimeout))
                {
                    printf("Change External Time Base Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change External Time Base Settings Failed\n\n");
                }
            }

            if (eOperation == CInput::ChangeProcessingActionsSettings)
            {
                bool         bEnabled;
                int          nSignalSource;
                bool         bSignalModePeriodic;
                unsigned int nMultiplier;
                unsigned int nDivisor;
                unsigned int nFrequencyTolerance;
                float        fNominalFrequency;
                bool         bNegativeEdge;
                unsigned int nSignalShutterDelay;
                float        fNonPeriodicTimeout;
                CRTProtocol::EProcessingActions eProcessingActions = CRTProtocol::ProcessingNone;
                CRTProtocol::EProcessingActions eRtProcessingActions = CRTProtocol::ProcessingNone;
                CRTProtocol::EProcessingActions eReprocessingActions = CRTProtocol::ProcessingNone;

                mpoInput->ReadProcessingActionsSettings(eProcessingActions, eRtProcessingActions, eReprocessingActions);

                if (mpoRTProtocol->SetSystemSettings(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &eProcessingActions, &eRtProcessingActions, &eReprocessingActions))
                {
                    printf("Change General Settings Processing Actions Succeeded\n\n");
                }
                else
                {
                    printf("Change General Settings Processing Actions Failed\n\n");
                }


                if (mpoRTProtocol->SetExtTimeBaseSettings(&bEnabled, (CRTProtocol::ESignalSource*)&nSignalSource,
                    &bSignalModePeriodic, &nMultiplier,       &nDivisor,      
                    &nFrequencyTolerance, &fNominalFrequency, &bNegativeEdge,
                    &nSignalShutterDelay, &fNonPeriodicTimeout))
                {
                    printf("Change External Time Base Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change External Time Base Settings Failed\n\n");
                }
            }

            if (eOperation == CInput::ChangeCameraSettings)
            {
                printf("\n\nInput Camera Settings\n\n");

                int           nMode;
                int           nVideoMode       = -1;
                unsigned int  nVideoFrequency  = -1;
                float         fVideoExposure   = -1;
                float         fVideoFlashTime  = -1;
                float         fMarkerExposure  = -1;
                float         fMarkerThreshold = -1;
                int           nRotation;
                int*          pnVideoMode;
                unsigned int* pnVideoFrequency;
                float*        pfVideoExposure;
                float*        pfVideoFlashTime;
                float*        pfMarkerExposure;
                float*        pfMarkerThreshold;

                mpoInput->ReadCameraSettings(nCameraId, nMode, nVideoMode, nVideoFrequency, 
                                             fVideoExposure, fVideoFlashTime, fMarkerExposure,
                                             fMarkerThreshold, nRotation);

                pnVideoMode       = (nVideoMode == -1) ? NULL : &nVideoMode;
                pnVideoFrequency  = (nVideoFrequency == -1) ? NULL : &nVideoFrequency;
                pfVideoExposure   = (fVideoExposure == -1) ? NULL : &fVideoExposure;
                pfVideoFlashTime  = (fVideoFlashTime == -1) ? NULL : &fVideoFlashTime;
                pfMarkerExposure  = (fMarkerExposure == -1) ? NULL : &fMarkerExposure;
                pfMarkerThreshold = (fMarkerThreshold == -1) ? NULL : &fMarkerThreshold;

                if (mpoRTProtocol->SetCameraSettings(nCameraId, (CRTProtocol::ECameraMode*)&nMode,
                                                     pfMarkerExposure, pfMarkerThreshold, &nRotation))
                {
                    printf("Change Camera Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change Camera Settings Failed\n\n");
                }

                if (mpoRTProtocol->SetCameraVideoSettings(nCameraId, (CRTProtocol::ECameraVideoMode*)pnVideoMode,
                                                          pnVideoFrequency, pfVideoExposure, pfVideoFlashTime))
                {
                    printf("Change Camera Video Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change Camera Video Settings Failed\n\n");
                }
            }

            if (eOperation == CInput::ChangeCameraSyncOutSettings)
            {
                printf("\n\nInput Camera Sync Out Settings\n\n");

                int          portNumber;
                int          nSyncOutMode;
                unsigned int nSyncOutValue;
                float        fSyncOutDutyCycle;
                bool         bSyncOutNegativePolarity;

                mpoInput->ReadCameraSyncOutSettings(nCameraId, portNumber, nSyncOutMode, nSyncOutValue, fSyncOutDutyCycle,
                                                     bSyncOutNegativePolarity);

                if (mpoRTProtocol->SetCameraSyncOutSettings(nCameraId, portNumber, (CRTProtocol::ESyncOutFreqMode*)&nSyncOutMode,
                                                           &nSyncOutValue, &fSyncOutDutyCycle,
                                                           &bSyncOutNegativePolarity))
                {
                    printf("Change Camera Sync Out Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change Camera Syn Out Settings Failed\n\n");
                }
            }

            if (eOperation == CInput::ChangeImageSettings)
            {
                printf("\n\nInput Image Settings\n\n");

                mpoInput->ReadImageSettings(nCameraId, bEnable, nImageFormat, nWidth, nHeight,
                                            fLeftCrop, fTopCrop, fRightCrop, fBottomCrop);

                if (mpoRTProtocol->SetImageSettings(nCameraId, &bEnable, (CRTPacket::EImageFormat*)&nImageFormat, &nWidth,
                                                    &nHeight, &fLeftCrop, &fTopCrop, &fRightCrop, &fBottomCrop))
                {
                    printf("Change Image Settings Succeeded\n\n");
                }
            }

            if (eOperation == CInput::ChangeForceSettings)
            {
                float        aCorner[4][3];
                unsigned int nPlateID;

                printf("\n\nInput Force Settings\n\n");

                mpoInput->ReadForceSettings(nPlateID, aCorner);

                if (mpoRTProtocol->SetForceSettings(nPlateID,
                                                    (CRTProtocol::SPoint*)aCorner[0], (CRTProtocol::SPoint*)aCorner[1],
                                                    (CRTProtocol::SPoint*)aCorner[2], (CRTProtocol::SPoint*)aCorner[3]))
                {
                    printf("Change Force Settings Succeeded\n\n");
                }
            }

            if (mpoRTProtocol->ReleaseControl() == false)
            {
                printf("Failed to release QTM control.\n\n");
            }
        }
        else
        {
            if (strncmp("Wrong or missing password", mpoRTProtocol->GetErrorString(), 25) == 0)
            {
                mpoInput->ReadClientControlPassword(pPassword, sizeof(pPassword));
                printf("\n");
            }
            else
            {
                printf("Failed to take control over QTM. %s\n\n", mpoRTProtocol->GetErrorString());
            }
        }
    } while (!bGotControl && pPassword[0] != 0);

} // ChangeSettings


void COperations::DataTransfer(CInput::EOperation operation)
{
    CRTPacket::EPacketType      ePacketType;
    unsigned int                nComponentType;
    char                        selectedAnalogChannels[256];
    CRTProtocol::EStreamRate    eStreamRate;
    int                         nRateArgument;
    FILE*                       logfile = NULL;
    bool                        bStreamTCP, bStreamUDP, bLogToFile, bOnlyTimeAndFrameNumber;
    unsigned short              nUDPPort;
    char                        tUDPAddress[256];
    bool                        bOutputModeScrolling = false;

    mpoOutput->ResetCounters();

    if (operation == CInput::Noise2D)
    {
        nComponentType = CRTProtocol::cComponent2d;
        bStreamTCP = true;
        bStreamUDP = false;
        bLogToFile = false;
        bOnlyTimeAndFrameNumber = false;
        nUDPPort = 0;
        tUDPAddress[0] = 0;
        eStreamRate = CRTProtocol::RateAllFrames;
        nRateArgument = 0;

        mpoOutput->Reset2DNoiseCalc();
    }
    else
    {
        if (!mpoInput->ReadDataComponents(nComponentType, selectedAnalogChannels, sizeof(selectedAnalogChannels)))
        {
            return;
        }

        if (!mpoInput->ReadDataTest(operation != CInput::Statistics, bStreamTCP, bStreamUDP, bLogToFile, bOnlyTimeAndFrameNumber, nUDPPort, tUDPAddress, sizeof(tUDPAddress)))
        {
            return;
        }
        if (operation == CInput::Statistics)
        {
            eStreamRate = CRTProtocol::RateAllFrames;
        }
        else if (bStreamTCP || bStreamUDP)
        {
            if (!mpoInput->ReadStreamRate(eStreamRate, nRateArgument))
            {
                return;
            }
        }
    }

    if (bLogToFile)
    {
        char pFileName[256];

        mpoInput->ReadFileName(pFileName, sizeof(pFileName));

        if (fopen_s(&logfile, pFileName, "w") != 0)
        {
            printf("\n\nOpen/Create log file failed.\n");
            return;
        }
    }
    else
    {
        logfile = stdout;
        if (!mpoInput->ReadUseScrolling(bOutputModeScrolling))
        {
            return;
        }
    }

    // Clear screen
    system("cls");

    if (logfile != stdout)
    {
        printf("\nMeasurement started. Press any key to abort.");
    }

    mpoRTProtocol->ReadCameraSystemSettings();

    bool bDataAvailable;

    if ((nComponentType & CRTProtocol::cComponent3d)         |
        (nComponentType & CRTProtocol::cComponent3dRes)      |
        (nComponentType & CRTProtocol::cComponent3dNoLabels) |
        (nComponentType & CRTProtocol::cComponent3dNoLabelsRes))
    {
        mpoRTProtocol->Read3DSettings(bDataAvailable);
    }

    if ((nComponentType & CRTProtocol::cComponent6d)      |
        (nComponentType & CRTProtocol::cComponent6dRes)   |
        (nComponentType & CRTProtocol::cComponent6dEuler) |
        (nComponentType & CRTProtocol::cComponent6dEulerRes))
    {
        mpoRTProtocol->Read6DOFSettings(bDataAvailable);
    }

    if ((nComponentType & CRTProtocol::cComponentGazeVector))
    {
        mpoRTProtocol->ReadGazeVectorSettings(bDataAvailable);
    }

    if ((nComponentType & CRTProtocol::cComponentAnalog) |
        (nComponentType & CRTProtocol::cComponentAnalogSingle))
    {
        mpoRTProtocol->ReadAnalogSettings(bDataAvailable);
    }

    if ((nComponentType & CRTProtocol::cComponentForce) |
        (nComponentType & CRTProtocol::cComponentForceSingle))
    {
        mpoRTProtocol->ReadForceSettings(bDataAvailable);
    }

    if (nComponentType == CRTProtocol::cComponentImage)
    {
        if (mpoRTProtocol->ReadImageSettings(bDataAvailable))
        {
            if (TakeQTMControl())
            {
                unsigned int nCameraId         = 1;
                CRTProtocol::ECameraMode nMode = CRTProtocol::ModeVideo;

                CRTPacket::EEvent eEvent;
                bool              bConnected = true;

                mpoRTProtocol->GetState(eEvent);
                if (eEvent == CRTPacket::EventConnectionClosed)
                {
                    bConnected = false;
                    if (mpoRTProtocol->NewMeasurement())
                    {
                        bConnected = true;
                    }
                }
                
                if (bConnected)
                {
                    if (mpoRTProtocol->SetCameraSettings(nCameraId, &nMode, NULL, NULL, NULL))
                    {
                        CRTPacket::EImageFormat nImageFormat = CRTPacket::FormatJPG;
                        bool bEnable;
                        unsigned int nWidth;
                        unsigned int nHeight;
                        float fLeftCrop;
                        float fTopCrop;
                        float fRightCrop;
                        float fBottomCrop;

                        mpoInput->ReadImageSettings(nCameraId, bEnable, (int&)nImageFormat, nWidth, nHeight,
                            fLeftCrop, fTopCrop, fRightCrop, fBottomCrop);

                        if (mpoRTProtocol->SetImageSettings(nCameraId, &bEnable, &nImageFormat, &nWidth,
                            &nHeight, &fLeftCrop, &fTopCrop, &fRightCrop, &fBottomCrop) == false)
                        {
                            printf("Change Image Settings Failed. %s\n\n", mpoRTProtocol->GetErrorString());
                        }
                    }
                    else
                    {
                        printf("Change General Settings Failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                }
                else
                {
                    printf("Unable to start new measurement. %s\n\n", mpoRTProtocol->GetErrorString());
                }

                if (ReleaseQTMControl() == false)
                {
                    return;
                }
            }
        }
    }

    bool bAbort = false;

    if (bStreamTCP || bStreamUDP)
    {
        // Start streaming data frames.
        if (bStreamUDP)
        {
            char* pAddr = NULL;
            unsigned short nPort = mpoRTProtocol->GetUdpServerPort();
            if (nUDPPort > 0)
            {
                nPort = nUDPPort;
            }
            if (strlen(tUDPAddress) > 0)
            {
                pAddr = tUDPAddress;
            }
            bAbort = !mpoRTProtocol->StreamFrames(eStreamRate, nRateArgument, nPort, pAddr, nComponentType, selectedAnalogChannels);
        }
        else
        {
            bAbort = !mpoRTProtocol->StreamFrames(eStreamRate, nRateArgument, 0, NULL, nComponentType, selectedAnalogChannels);
        }
    }

    // Main data read loop
    while (!bAbort)
    {
        if (!bStreamTCP && !bStreamUDP)
        {
            bAbort = (mpoRTProtocol->GetCurrentFrame(nComponentType, selectedAnalogChannels) == false);
        }

        if (mpoRTProtocol->ReceiveRTPacket(ePacketType, true) > 0)
        {
            switch (ePacketType) 
            {
                case CRTPacket::PacketError : // sHeader.nType 0 indicates an error
                    {
                        CRTPacket* poRTPacket = mpoRTProtocol->GetRTPacket();
                        if (bStreamTCP || bStreamUDP)
                        {
                            fprintf(stderr, "Error at StreamFrames: %s\n", poRTPacket->GetErrorString());
                        }
                        else
                        {
                            fprintf(stderr, "Error at SendCurrentFrame: %s\n", poRTPacket->GetErrorString());
                        }
                        break;
                    }
                case CRTPacket::PacketData:         // Data received
                    mpoOutput->HandleDataFrame(logfile, bOnlyTimeAndFrameNumber, mpoRTProtocol, operation, bOutputModeScrolling);
                    break;
                case CRTPacket::PacketNoMoreData :   // No more data
                    break;
                default:
                    break;
            }
        }
        bAbort = mpoInput->CheckKeyPressed();
    }

    if (bStreamTCP || bStreamUDP)
    {
        mpoRTProtocol->StreamFramesStop();
    }

    if (operation != CInput::Noise2D && (operation != CInput::Statistics))
    {
        mpoOutput->PrintTimingData();
    }

    if (logfile != stdout && logfile != NULL)
    {
        fclose(logfile);
    }

} // DataTransfer

void COperations::ControlQTM()
{
    CInput::EQTMCommand eCommand;
    char                pFileName[256];

    // Take control over QTM
    if (TakeQTMControl())
    {
        while (mpoInput->ReadQTMCommand(eCommand))
        {
            switch (eCommand)
            {
                case CInput::New :
                    if (mpoRTProtocol->NewMeasurement())
                    {
                        printf("Creating new connection.\n\n");
                    }
                    else
                    {
                        printf("New failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Close :
                    if (mpoRTProtocol->CloseMeasurement())
                    {
                        printf("Closing connection.\n\n");
                    }
                    else
                    {
                        printf("Close failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Start :
                    if (mpoRTProtocol->StartCapture())
                    {
                        printf("Starting measurement.\n\n");
                    }
                    else
                    {
                        printf("Start failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::StartRTFromFile :
                    if (mpoRTProtocol->StartRTOnFile())
                    {
                        printf("Starting RT from file.\n\n");
                    }
                    else
                    {
                        printf("Start RT from file failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Stop :
                    if (mpoRTProtocol->StopCapture())
                    {
                        printf("Stopping measurement\n\n");
                    }
                    else
                    {
                        printf("Stop failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Load :
                    mpoInput->ReadFileName(pFileName, sizeof(pFileName));
                    if (mpoRTProtocol->LoadCapture(pFileName))
                    {
                        printf("Measurement loaded.\n\n");
                    }
                    else
                    {
                        printf("Load failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Save :
                    {
                        mpoInput->ReadFileName(pFileName, sizeof(pFileName));
                        printf("Overwrite existing measurement (y/n)? ");
                        bool bOverWrite = mpoInput->ReadYesNo(false);
                        char tNewFileName[300];
                        if (mpoRTProtocol->SaveCapture(pFileName, bOverWrite, tNewFileName, sizeof(tNewFileName)))
                        {
                            if (strlen(tNewFileName) == 0)
                            {
                                printf("Measurement saved.\n\n");
                            }
                            else
                            {
                                printf_s("Measurement saved as : %s.\n\n", tNewFileName);
                            }
                        }
                        else
                        {
                            printf("Save failed. %s\n\n", mpoRTProtocol->GetErrorString());
                        }
                    }
                    break;
                case CInput::LoadProject :
                    mpoInput->ReadFileName(pFileName, sizeof(pFileName));
                    if (mpoRTProtocol->LoadProject(pFileName))
                    {
                        printf("Project loaded.\n\n");
                    }
                    else
                    {
                        printf("Load failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::GetCaptureC3D :
                    mpoInput->ReadFileName(pFileName, sizeof(pFileName));
                    if (mpoRTProtocol->GetCapture(pFileName, true))
                    {
                        printf("C3D file written successfully to : %s.\n\n", pFileName);
                    }
                    else
                    {
                        printf("GetCaptureC3D failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::GetCaptureQTM :
                    mpoInput->ReadFileName(pFileName, sizeof(pFileName));
                    if (mpoRTProtocol->GetCapture(pFileName, false))
                    {
                        printf("QTN file written successfully to : %s.\n\n", pFileName);
                    }
                    else
                    {
                        printf("GetCaptureQTM failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Trig :
                    if (mpoRTProtocol->SendTrig())
                    {
                        printf("Trig sent\n\n");
                    }
                    else
                    {
                        printf("Trig failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::SetQTMEvent :
                    mpoInput->ReadFileName(pFileName, sizeof(pFileName));
                    if (mpoRTProtocol->SetQTMEvent(pFileName))
                    {
                        printf("QTM event set.\n\n");
                    }
                    else
                    {
                        printf("SetQTMEvent failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Reprocess:
                    if (mpoRTProtocol->Reprocess())
                    {
                        printf("Reprocess sent.\n\n");
                    }
                    else
                    {
                        printf("Reprocess failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                default :
                    break;
            }
        }
    }
} // ControlQTM


bool COperations::TakeQTMControl()
{
    char pPassword[256];

    pPassword[0] = 0;

    do 
    {
        // Take control over QTM
        if (mpoRTProtocol->TakeControl(pPassword))
        {
            return true;
        }
        else
        {
            if (strncmp("Wrong or missing password", mpoRTProtocol->GetErrorString(), 25) == 0)
            {
                mpoInput->ReadClientControlPassword(pPassword, sizeof(pPassword));
                printf("\n");
            }
            else
            {
                printf("Failed to take control over QTM. %s\n\n", mpoRTProtocol->GetErrorString());
            }
        }
    } while (pPassword[0] != 0);

    return false;
} // TakeQTMControl


bool COperations::ReleaseQTMControl()
{
    if (mpoRTProtocol->ReleaseControl() == false)
    {
        printf("Failed to release control over QTM.\n\n");
        return false;
    }
    return true;
} // ReleaseQTMControl