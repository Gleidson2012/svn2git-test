//============================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//----------------------------------------------------------------------------
#include <Appearance.h>
#include <Types.h>
#include <Controls.h>
#include <Events.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Sound.h>
#include <string.h>
#include <Resources.h>
#include <NumberFormatting.h>
#include <TextUtils.h>
#include <Files.h>
#include <string>
#include "duck_dxl.h"
#include "regentry.h"
#include "dxlqt_codec.h"
#include "GetSysFolder.h"
#include "common.h"
#include "type_aliases.h"


// constants for DITL vals
const int dxlqtConfigDialogID = 20000;
const int AutoKeyThresholdSliderID = 1;
const int MinDistToKeySliderID = 2;
const int ForceKeyFrameEverySliderID = 3;
const int CheckAutoKeyID = 12;
const int CheckAllowDFID = 4;
const int CheckQuickCompressID = 5;
const int AutoKeyTextBoxID = 6;
const int MinDistTextBoxID = 7;
const int ForceKeyTextBoxID = 8;
const int ButtonOKID = 9;
const int ButtonCancelID = 10;	
const int ButtonDefaultID = 11;

class dxlqtConfigDialog {
private:
    int 	            NoiseSensitivity;				// Preprocessor Setting in dialog
    int 	            KeyFrameDataTarget;
    int 	            AutoKeyFrameThreshold;			// %diff key frame threshold 
    int 	            MinimumDistanceToKeyFrame;		// min and max dist between key frames
    int 	            ForceKeyFrameEvery;	 		
    
    int                 lastControlEditID;
    int                 kMaxEditLength;
    
    Boolean  		    AutoKeyFrameEnabled;			// Auto scene change detect
    Boolean 		    AllowDF;						// dropped frames
    Boolean 		    QuickCompress;
    Boolean             AllowWavelet;                   // not a user setting, internal
    Boolean	 	        applySettings;					// false unless user has hit OK to confirm settings
    Boolean 		    Finished;						// dialog finished?
    Boolean             DialogFirstRunFlag;
    Boolean             CodecFirstRunFlag;
	
    void                InitSettings();
    void 			    DisplayDialog();
    void			    SetDefaults();
    void 			    InitDialogControlSettings();
    void			    updateSettings();
    
    int				    GetTextBoxValue(DialogItemIndex textBoxID);
    void                InitEditControl(DialogPtr theDialog, short theItem, 
                                            unsigned int theValue);    
    Boolean             ValidateCurrentEntry(DialogPtr theDialog, DialogItemIndex textBoxID, 
                                            int theValue);                                           
    void                UpdateSlider(DialogPtr theDialog, DialogItemIndex textBoxID);
    void                UpdateItemText(DialogPtr theDialog, short theItem, int theValue);      
    
    // OS X Special Stuff
    void                EmbedControls();
    
    // These are needed for TextBox input, they are turned on and off based upon
    // the user's interaction with textbox.  First keypress in a box and the corresponding
    // flag will be set to true.  On the second hit the flag is turned off and validation
    // will occur.  This is a kludge I'm using to make sure we don't bark about a value
    // out of range when the user types the 1 in 10 (for example).
    bool                m_bAutoTextFirstHit;
    bool                m_bMinTextFirstHit;
    bool                m_bForceTextFirstHit;
   
    
public:
                        dxlqtConfigDialog(dxlqt_Globals glob);		
    short               myResFile;
    void                osxErrorMsg(std::string& title, std::string& msg);
    DialogPtr           m_theDialog;
    COMP_CONFIG* 	    Settings;						// internal Ptr to user settings
    dxlqt_Globals       internalGlob;
};
