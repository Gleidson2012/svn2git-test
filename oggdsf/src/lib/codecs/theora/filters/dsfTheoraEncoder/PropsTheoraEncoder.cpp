#include "stdafx.h"
#include "propstheoraencoder.h"

PropsTheoraEncoder::PropsTheoraEncoder(LPUNKNOWN inUnk, HRESULT* outHR)
	:	CBasePropertyPage(NAME("illiminable Directshow Filters"), inUnk, IDD_THEORA_ENCODE_SETTINGS, IDS_THEORA_ENC_PROPS_STRING)
	,	mTheoraEncodeSettings(NULL)

{
	//debugLog.open("G:\\logs\\TheoProps.log", ios_base::out);
	*outHR = S_OK;
}

PropsTheoraEncoder::~PropsTheoraEncoder(void)
{
	//debugLog.close();
}

CUnknown* PropsTheoraEncoder::CreateInstance(LPUNKNOWN inUnk, HRESULT* outHR)
{
    return new PropsTheoraEncoder(inUnk, outHR);
}

//LRESULT PropsTheoraEncoder::addNumberToCombo(int inComboID, int inNum) {
//	char locStrBuff[16];
//	itoa(inNum, (char*)&locStrBuff, 10);
//	return SendDlgItemMessage(m_Dlg, IDC_COMBO_BITRATE, CB_ADDSTRING, NOT_USED, (LPARAM)&locStrBuff);
//
//}
//
//void PropsTheoraEncoder::SetupBitrateCombo() {
//	addNumberToCombo(IDC_COMBO_BITRATE, 64000);
//	addNumberToCombo(IDC_COMBO_BITRATE, 96000);
//	addNumberToCombo(IDC_COMBO_BITRATE, 128000);
//	addNumberToCombo(IDC_COMBO_BITRATE, 192000);
//	addNumberToCombo(IDC_COMBO_BITRATE, 256000);
//	addNumberToCombo(IDC_COMBO_BITRATE, 384000);
//	addNumberToCombo(IDC_COMBO_BITRATE, 512000);
//	addNumberToCombo(IDC_COMBO_BITRATE, 768000);
//	addNumberToCombo(IDC_COMBO_BITRATE, 1024000);
//	addNumberToCombo(IDC_COMBO_BITRATE, 1536000);
//	addNumberToCombo(IDC_COMBO_BITRATE, 2000000);
//
//}
//
//void PropsTheoraEncoder::SetupKeyframeFreqCombo() {
//	
//	addNumberToCombo(IDC_COMBO_LOG_KEYFRAME_FREQ, 1);
//	addNumberToCombo(IDC_COMBO_LOG_KEYFRAME_FREQ, 2);
//	addNumberToCombo(IDC_COMBO_LOG_KEYFRAME_FREQ, 3);
//	addNumberToCombo(IDC_COMBO_LOG_KEYFRAME_FREQ, 4);
//	addNumberToCombo(IDC_COMBO_LOG_KEYFRAME_FREQ, 5);
//	addNumberToCombo(IDC_COMBO_LOG_KEYFRAME_FREQ, 6);
//	addNumberToCombo(IDC_COMBO_LOG_KEYFRAME_FREQ, 7);
//	addNumberToCombo(IDC_COMBO_LOG_KEYFRAME_FREQ, 8);
//	addNumberToCombo(IDC_COMBO_LOG_KEYFRAME_FREQ, 9);
//	addNumberToCombo(IDC_COMBO_LOG_KEYFRAME_FREQ, 10);
//
//}

unsigned long PropsTheoraEncoder::log2(unsigned long inNum) {
	unsigned long ret = 0;
	while (inNum != 0) {
		inNum>>=1;
		ret++;
	}
	return ret - 1;
}

unsigned long PropsTheoraEncoder::pow2(unsigned long inNum) {
	return 1 << (inNum);
}

HRESULT PropsTheoraEncoder::OnApplyChanges(void)
{
	if (mTheoraEncodeSettings == NULL) {
		return E_POINTER;
	}

	mTheoraEncodeSettings->setQuality(SendDlgItemMessage(m_hwnd,IDC_SLIDER_QUALITY, TBM_GETPOS, NOT_USED, NOT_USED));
	mTheoraEncodeSettings->setKeyframeFreq(pow2(SendDlgItemMessage(m_hwnd,IDC_SLIDER_LOG_KEYFRAME, TBM_GETPOS, NOT_USED, NOT_USED)));
	mTheoraEncodeSettings->setTargetBitrate(SendDlgItemMessage(m_hwnd,IDC_SLIDER_BITRATE, TBM_GETPOS, NOT_USED, NOT_USED) * 1000);
	SetClean();
    return S_OK;
}

HRESULT PropsTheoraEncoder::OnActivate(void)
{
    
    wchar_t* locStrBuff = new wchar_t[16];

	//SetupBitrateCombo();
	//SetupKeyframeFreqCombo();
	
	//Set up the sliders
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_QUALITY, TBM_SETRANGE, TRUE, MAKELONG(0, 63));
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_QUALITY, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_QUALITY, TBM_SETPOS, 1, mTheoraEncodeSettings->quality());

	SendDlgItemMessage(m_Dlg, IDC_SLIDER_LOG_KEYFRAME, TBM_SETRANGE, TRUE, MAKELONG(0, 13));
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_LOG_KEYFRAME, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_LOG_KEYFRAME, TBM_SETPOS, 1, log2(mTheoraEncodeSettings->keyframeFreq()));

	SendDlgItemMessage(m_Dlg, IDC_SLIDER_LOG_KEYFRAME_MIN, TBM_SETRANGE, TRUE, MAKELONG(0, 13));
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_LOG_KEYFRAME_MIN, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_LOG_KEYFRAME_MIN, TBM_SETPOS, 1, log2(mTheoraEncodeSettings->keyframeFreqMin()));

	SendDlgItemMessage(m_Dlg, IDC_SLIDER_BITRATE, TBM_SETRANGE, TRUE, MAKELONG(64, 3968));
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_BITRATE, TBM_SETTICFREQ, 32, 0);
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_BITRATE, TBM_SETPOS, 1, mTheoraEncodeSettings->targetBitrate() / 1000);

	SendDlgItemMessage(m_Dlg, IDC_SLIDER_BITRATE_KEYFRAME, TBM_SETRANGE, TRUE, MAKELONG(64, 7936));
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_BITRATE_KEYFRAME, TBM_SETTICFREQ, 32, 0);
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_BITRATE_KEYFRAME, TBM_SETPOS, 1, mTheoraEncodeSettings->keyFrameDataBitrate() / 1000);

	SendDlgItemMessage(m_Dlg, IDC_SLIDER_KF_THRESHOLD, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_KF_THRESHOLD, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessage(m_Dlg, IDC_SLIDER_KF_THRESHOLD, TBM_SETPOS, 1, log2(mTheoraEncodeSettings->keyframeAutoThreshold()));






	_itow(mTheoraEncodeSettings->quality(), locStrBuff, 10);
	SendDlgItemMessage(m_Dlg, IDC_LABEL_QUALITY, WM_SETTEXT, NOT_USED, (LPARAM)locStrBuff);

	_itow(mTheoraEncodeSettings->keyframeFreq(), locStrBuff, 10);
	SendDlgItemMessage(m_Dlg, IDC_LABEL_LOG_KEYFRAME, WM_SETTEXT,NOT_USED,  (LPARAM)locStrBuff);

	_itow(mTheoraEncodeSettings->targetBitrate(), locStrBuff, 10);
	SendDlgItemMessage(m_Dlg, IDC_LABEL_BITRATE, WM_SETTEXT,NOT_USED,  (LPARAM)locStrBuff);

	_itow(mTheoraEncodeSettings->keyFrameDataBitrate(), locStrBuff, 10);
	SendDlgItemMessage(m_Dlg, IDC_LABEL_BITRATE_KEYFRAME, WM_SETTEXT,NOT_USED,  (LPARAM)locStrBuff);


	_itow(mTheoraEncodeSettings->keyframeFreqMin(), locStrBuff, 10);
	SendDlgItemMessage(m_Dlg, IDC_LABEL_LOG_KEYFRAME_MIN, WM_SETTEXT,NOT_USED,  (LPARAM)locStrBuff);


	_itow(mTheoraEncodeSettings->keyframeAutoThreshold(), locStrBuff, 10);
	SendDlgItemMessage(m_Dlg, IDC_LABEL_KF_THRESHOLD, WM_SETTEXT,NOT_USED,  (LPARAM)locStrBuff);




    wstring locListString = L"0 - Sharpest (default)";
    SendDlgItemMessage(m_Dlg, IDC_LIST_SHARPNESS, LB_ADDSTRING, NOT_USED,  (LPARAM)locListString.c_str());

    locListString = L"1 - Sharper";
    SendDlgItemMessage(m_Dlg, IDC_LIST_SHARPNESS, LB_ADDSTRING, NOT_USED,  (LPARAM)locListString.c_str());

    locListString = L"2 - Sharp (fastest)";
    SendDlgItemMessage(m_Dlg, IDC_LIST_SHARPNESS, LB_ADDSTRING, NOT_USED,  (LPARAM)locListString.c_str());


    SendDlgItemMessage(m_Dlg, IDC_LIST_SHARPNESS, LB_SETSEL, (WPARAM)mTheoraEncodeSettings->sharpness(), NOT_USED);

    locListString = L"0 - Most sensitive";
    SendDlgItemMessage(m_Dlg, IDC_LIST_NOISE_SENS, LB_ADDSTRING, NOT_USED,  (LPARAM)locListString.c_str());

    locListString = L"1 - Theora default";
    SendDlgItemMessage(m_Dlg, IDC_LIST_NOISE_SENS, LB_ADDSTRING, NOT_USED,  (LPARAM)locListString.c_str());

    locListString = L"2 - VP3 default";
    SendDlgItemMessage(m_Dlg, IDC_LIST_NOISE_SENS, LB_ADDSTRING, NOT_USED,  (LPARAM)locListString.c_str());

    locListString = L"3 - ";
    SendDlgItemMessage(m_Dlg, IDC_LIST_NOISE_SENS, LB_ADDSTRING, NOT_USED,  (LPARAM)locListString.c_str());

    locListString = L"4 - ";
    SendDlgItemMessage(m_Dlg, IDC_LIST_NOISE_SENS, LB_ADDSTRING, NOT_USED,  (LPARAM)locListString.c_str());

    locListString = L"5 - ";
    SendDlgItemMessage(m_Dlg, IDC_LIST_NOISE_SENS, LB_ADDSTRING, NOT_USED,  (LPARAM)locListString.c_str());

    locListString = L"6 - Least sensitive";
    SendDlgItemMessage(m_Dlg, IDC_LIST_NOISE_SENS, LB_ADDSTRING, NOT_USED,  (LPARAM)locListString.c_str());

    locListString = L"? - Fallback (2.5)";
    SendDlgItemMessage(m_Dlg, IDC_LIST_NOISE_SENS, LB_ADDSTRING, NOT_USED,  (LPARAM)locListString.c_str());

    SendDlgItemMessage(m_Dlg, IDC_LIST_NOISE_SENS, LB_SETCURSEL, (WPARAM)mTheoraEncodeSettings->noiseSensitivity(), NOT_USED);

	delete[] locStrBuff;
    return S_OK;
}

HRESULT PropsTheoraEncoder::OnConnect(IUnknown *pUnk)
{
    
	if (mTheoraEncodeSettings != NULL) {
		//mTheoraEncodeSettings->Release();
		mTheoraEncodeSettings = NULL;
	}

    HRESULT locHR;
    // Query pUnk for the filter's custom interface.
    locHR = pUnk->QueryInterface(IID_ITheoraEncodeSettings, (void**)(&mTheoraEncodeSettings));
    return locHR;
}

HRESULT PropsTheoraEncoder::OnDisconnect(void)
{
	if (mTheoraEncodeSettings != NULL) {
		//mTheoraEncodeSettings->Release();
		mTheoraEncodeSettings = NULL;
	}
    return S_OK;
}
void PropsTheoraEncoder::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
}

void PropsTheoraEncoder::SetClean()
{
    m_bDirty = FALSE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_CLEAN);
    }
}
INT_PTR PropsTheoraEncoder::OnReceiveMessage(HWND hwnd,  UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t locBuff[16];
    
    switch (uMsg)    {
		case WM_COMMAND:
            //TODO::: Need to check the high wparam ??
            if (HWND(lParam) == GetDlgItem(m_hwnd, IDC_FIXED_KFI_CHECK)) {
                if (SendDlgItemMessage(m_hwnd,IDC_FIXED_KFI_CHECK, BM_GETCHECK, NOT_USED, NOT_USED)) {
                    //Disable the extra stuff
                }
            } else if (HWND(lParam) == GetDlgItem(m_hwnd, IDC_CHECK_ALLOW_DROP_FRAMES)) {
                if (SendDlgItemMessage(m_hwnd,IDC_CHECK_ALLOW_DROP_FRAMES, BM_GETCHECK, NOT_USED, NOT_USED)) {
                    //Do we even need to catch this one?
                }
            }
            break;
		
		case WM_HSCROLL:
			if (HWND(lParam) == GetDlgItem(m_hwnd, IDC_SLIDER_QUALITY)) {
				SetDirty();
				_itow(SendDlgItemMessage(m_hwnd,IDC_SLIDER_QUALITY, TBM_GETPOS, NOT_USED, NOT_USED), locBuff, 10);
				SendDlgItemMessage(m_hwnd, IDC_LABEL_QUALITY, WM_SETTEXT, NOT_USED, (LPARAM)&locBuff);

                //TODO::: Kill the bitrate one
                return (INT_PTR)TRUE;

			} else if (HWND(lParam) == GetDlgItem(m_hwnd, IDC_SLIDER_BITRATE)) {
				SetDirty();
				_itow(SendDlgItemMessage(m_hwnd,IDC_SLIDER_BITRATE, TBM_GETPOS, NOT_USED, NOT_USED) * 1000, locBuff, 10);
				SendDlgItemMessage(m_hwnd, IDC_LABEL_BITRATE, WM_SETTEXT, NOT_USED, (LPARAM)&locBuff);

                //Kill the quailty one
                return (INT_PTR)TRUE;

			} else if (HWND(lParam) == GetDlgItem(m_hwnd, IDC_SLIDER_BITRATE_KEYFRAME)) {
				SetDirty();
				_itow(SendDlgItemMessage(m_hwnd,IDC_SLIDER_BITRATE_KEYFRAME, TBM_GETPOS, NOT_USED, NOT_USED) * 1000, locBuff, 10);
				SendDlgItemMessage(m_hwnd, IDC_SLIDER_BITRATE_KEYFRAME, WM_SETTEXT, NOT_USED, (LPARAM)&locBuff);

                //Kill the quailty one
                return (INT_PTR)TRUE;

			} else if (HWND(lParam) == GetDlgItem(m_hwnd, IDC_SLIDER_LOG_KEYFRAME)) {
				SetDirty();
				_itow(pow2(SendDlgItemMessage(m_hwnd,IDC_SLIDER_LOG_KEYFRAME, TBM_GETPOS, NOT_USED, NOT_USED)), locBuff, 10);
				SendDlgItemMessage(m_hwnd, IDC_LABEL_LOG_KEYFRAME, WM_SETTEXT, NOT_USED, (LPARAM)&locBuff);
                return (INT_PTR)TRUE;
			} else if (HWND(lParam) == GetDlgItem(m_hwnd, IDC_SLIDER_LOG_KEYFRAME_MIN)) {
				SetDirty();
				_itow(pow2(SendDlgItemMessage(m_hwnd,IDC_SLIDER_LOG_KEYFRAME_MIN, TBM_GETPOS, NOT_USED, NOT_USED)), locBuff, 10);
				SendDlgItemMessage(m_hwnd, IDC_LABEL_LOG_KEYFRAME_MIN, WM_SETTEXT, NOT_USED, (LPARAM)&locBuff);
                return (INT_PTR)TRUE;
            } else if (HWND(lParam) == GetDlgItem(m_hwnd, IDC_SLIDER_KF_THRESHOLD)) {
				SetDirty();
				_itow(pow2(SendDlgItemMessage(m_hwnd,IDC_SLIDER_KF_THRESHOLD, TBM_GETPOS, NOT_USED, NOT_USED)), locBuff, 10);
				SendDlgItemMessage(m_hwnd, IDC_LABEL_KF_THRESHOLD, WM_SETTEXT, NOT_USED, (LPARAM)&locBuff);
                return (INT_PTR)TRUE;
            }

			break;

         
    } // switch

    // Did not handle the message.
    return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}


