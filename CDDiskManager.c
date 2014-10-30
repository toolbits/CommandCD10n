/***      Command CD 1.0n  Command Line CD Player.**      Copyright (C) 1998 - 1999 Qwer**      Homepage  http://member.nifty.ne.jp/~Qwer**      Mail      zap00365@nifty.ne.jp****      This source code is for Think C.****		CDDiskManager.c*/#include <Folders.h>#include "CDDiskManager.h"#define			CDDriverName		"\p.AppleCD"#define			CDRemoteFileName	"\pCD Remote Programs"#define			CDRemoteType		'IndX'#define			CDRemoteID			128#define			DefaultDiskName		"\pAudio CD"#define			DefaultTrackName	"\pTrack "#define			CDMaxTrack			99typedef struct CDDiskAddress{	short				Minute;	short				Second;	short				Frame;} CDDiskAddress,*CDDiskAddressPtr,**CDDiskAddressHandle;typedef struct CDDiskTOC{	short				DiskMaxTrack;	CDDiskAddress		DiskTotalTime;	CDDiskAddress		LeadOut;	CDDiskAddress		TrackStart[CDMaxTrack];	CDDiskAddress		TrackTime[CDMaxTrack];	long				TrackTotalFrame[CDMaxTrack];} CDDiskTOC,*CDDiskTOCPtr,**CDDiskTOCHandle;typedef struct CDNameTOC{	Str255				DiskName;	Str255				TrackName[CDMaxTrack];} CDNameTOC,*CDNameTOCPtr,**CDNameTOCHandle;typedef struct CDQSubcode{	short				CurrentStatus;	short				PlayMode;	short				ControlField;	short				CurrentTrack;	CDDiskAddress		DiskTime;	CDDiskAddress		TrackTime;} CDQSubcode,*CDQSubcodePtr,**CDQSubcodeHandle;//GlobalRoutinesOSErr			CDOpenDriver		(short,short*);OSErr			CDDiskInDrive		(Boolean*,short);OSErr			CDEjectDisk			(short);OSErr			CDReadDiskTOC		(CDDiskTOCPtr,short);OSErr			CDReadNameTOC		(CDDiskTOCPtr,CDNameTOCPtr);OSErr			CDReadTheQSubcode	(CDQSubcodePtr,short);OSErr			CDAudioTrackSearch	(CDDiskAddressPtr,Boolean,short);OSErr			CDAudioPlay			(CDDiskAddressPtr,CDDiskAddressPtr,short);OSErr			CDAudioPause		(Boolean,short);OSErr			CDAudioStop			(short);OSErr			CDAudioScan			(short,short);OSErr			CDAudioControl		(short,short,short);OSErr			CDReadAudioVolume	(short*,short*,short);OSErr			CDGetSpindleSpeed	(short*,short);OSErr			CDSetSpindleSpeed	(short,short);OSErr			CDReadAudioData		(CDDiskAddressPtr,Ptr,long,short);//StaticRoutinesstatic	void	MemoryClear			(Ptr,long);static	Byte	DecimalToBCD		(Byte);static	Byte	BCDToDecimal		(Byte);static	void	AddDiskAddress		(CDDiskAddressPtr,CDDiskAddressPtr,CDDiskAddressPtr);static	void	SubtractDiskAddress	(CDDiskAddressPtr,CDDiskAddressPtr,CDDiskAddressPtr);static	void	DiskAddressToFrame	(CDDiskAddressPtr,long*);static	void	CopyStringByteX		(unsigned char*,unsigned char*);static	void	ConnectStringByteX	(unsigned char*,unsigned char*);OSErr CDOpenDriver(short DriveIndex,short *Index){	CDDriveParam		CDParam;	short				CDDriveIndex;	short				ID;	Boolean				CanAccess;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	if((ResultCode = OpenDriver(CDDriverName,Index)) == noErr){		CDParam.Default.ioCRefNum = *Index;		CDParam.Default.csCode = 97;				if((ResultCode = PBStatus((ParmBlkPtr)&CDParam,false)) == noErr){			CDDriveIndex = 0;			CanAccess = false;			for(ID=0;ID<7;ID++){				if(BitTst(&CDParam.Option.DriveWholesThere.SCSIMask,7-ID) == true){					CDDriveIndex++;					if(DriveIndex == CDDriveIndex){						*Index = -(32 + ID) - 1;						CanAccess = true;						break;					}				}			}			if(CanAccess == false) ResultCode = paramErr;		}	}	return(ResultCode);}/*CDOpenDriver*/OSErr CDDiskInDrive(Boolean *DiskInDrive,short Index){	CDDriveParam		CDParam;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 8;	if((ResultCode = PBStatus((ParmBlkPtr)&CDParam,false)) == noErr) *DiskInDrive = (CDParam.Option.DriveStatus.DiskInDriveBit == 0) ? false : true;	return(ResultCode);}/*CDDiskInDrive*/OSErr CDEjectDisk(short Index){	HVolumeParam		CDParam;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(HVolumeParam));	CDParam.ioNamePtr = nil;	CDParam.ioVolIndex = 0;	do{		CDParam.ioVolIndex++;		ResultCode = PBHGetVInfo((HParmBlkPtr)&CDParam,false);	}while(CDParam.ioVDRefNum != Index && ResultCode == noErr);		if(ResultCode == noErr) if((ResultCode = PBEject((ParmBlkPtr)&CDParam)) == noErr) ResultCode = PBUnmountVol((ParmBlkPtr)&CDParam);	return(ResultCode);}/*CDEjectDisk*/OSErr CDReadDiskTOC(CDDiskTOCPtr DiskTOC,short Index){	CDDriveParam		CDParam;	CDDiskAddress		TempTime;	Ptr					RamTOC;	long				RamFix;	short				ID;	Byte				MinTrack;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 100;	CDParam.Option.ReadTOC1In.RequestType = 1;		if((ResultCode = PBControl((ParmBlkPtr)&CDParam,false)) == noErr){		DiskTOC->DiskMaxTrack = BCDToDecimal(CDParam.Option.ReadTOC1Out.MaxTrack);		MinTrack = CDParam.Option.ReadTOC1Out.MinTrack;				MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));		CDParam.Default.ioCRefNum = Index;		CDParam.Default.csCode = 100;		CDParam.Option.ReadTOC2In.RequestType = 2;				if((ResultCode = PBControl((ParmBlkPtr)&CDParam,false)) == noErr){								DiskTOC->LeadOut.Minute = BCDToDecimal(CDParam.Option.ReadTOC2Out.DiskMinute);			DiskTOC->LeadOut.Second = BCDToDecimal(CDParam.Option.ReadTOC2Out.DiskSecond);			DiskTOC->LeadOut.Frame = BCDToDecimal(CDParam.Option.ReadTOC2Out.DiskFrame);						if((RamTOC = NewPtrClear(512)) != nil){				MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));				CDParam.Default.ioCRefNum = Index;				CDParam.Default.csCode = 100;				CDParam.Option.ReadTOC3.RequestType = 3;				CDParam.Option.ReadTOC3.Address = RamTOC;				CDParam.Option.ReadTOC3.RequestByte = 512;				CDParam.Option.ReadTOC3.FirstTrack = MinTrack;								if((ResultCode = PBControl((ParmBlkPtr)&CDParam,false)) == noErr){					RamFix = 0;					ID = 0;					while(ID < DiskTOC->DiskMaxTrack){						if(RamTOC[RamFix++] & 0x04){							TempTime.Minute = 2;							TempTime.Second = 32;							TempTime.Frame = 0;							DiskTOC->LeadOut.Minute = BCDToDecimal(RamTOC[RamFix++]);							DiskTOC->LeadOut.Second = BCDToDecimal(RamTOC[RamFix++]);							DiskTOC->LeadOut.Frame = BCDToDecimal(RamTOC[RamFix++]);							SubtractDiskAddress(&DiskTOC->LeadOut,&TempTime,&DiskTOC->LeadOut);							break;						}						else{							DiskTOC->TrackStart[ID].Minute = BCDToDecimal(RamTOC[RamFix++]);							DiskTOC->TrackStart[ID].Second = BCDToDecimal(RamTOC[RamFix++]);							DiskTOC->TrackStart[ID].Frame = BCDToDecimal(RamTOC[RamFix++]);							ID++;						}					}					DiskTOC->DiskMaxTrack = ID;										if(DiskTOC->DiskMaxTrack > 0){						for(ID=0;ID<DiskTOC->DiskMaxTrack-1;ID++) SubtractDiskAddress(&DiskTOC->TrackStart[ID+1],&DiskTOC->TrackStart[ID],&DiskTOC->TrackTime[ID]);						SubtractDiskAddress(&DiskTOC->LeadOut,&DiskTOC->TrackStart[ID],&DiskTOC->TrackTime[ID]);						SubtractDiskAddress(&DiskTOC->LeadOut,&DiskTOC->TrackStart[0],&DiskTOC->DiskTotalTime);						for(ID=0;ID<DiskTOC->DiskMaxTrack;ID++) DiskAddressToFrame(&DiskTOC->TrackTime[ID],&DiskTOC->TrackTotalFrame[ID]);					}				}				DisposePtr(RamTOC);			}			else ResultCode = MemError();		}	}	return(ResultCode);}/*CDReadDiskTOC*/OSErr CDReadNameTOC(CDDiskTOCPtr DiskTOC,CDNameTOCPtr NameTOC){	CDRemoteIndexHandle		CDRemoteIndex;	FSSpec					TargetFile;	long					KeyCode;	short					SaveResFile;	short					CDRemoteResFile;	short					RecordID;	short					ID;	Str255					TrackIndex;	OSErr					ResultCode;		ResultCode = noErr;	SaveResFile = CurResFile();	CopyStringByteX(DefaultDiskName,NameTOC->DiskName);	for(ID=0;ID<DiskTOC->DiskMaxTrack;ID++){		NumToString(ID+1,TrackIndex);		CopyStringByteX(DefaultTrackName,NameTOC->TrackName[ID]);		ConnectStringByteX(TrackIndex,NameTOC->TrackName[ID]);	}		if((ResultCode = FindFolder(kOnSystemDisk,kPreferencesFolderType,kDontCreateFolder,&TargetFile.vRefNum,&TargetFile.parID)) == noErr){		CopyStringByteX(CDRemoteFileName,TargetFile.name);		CDRemoteResFile = FSpOpenResFile(&TargetFile,fsRdPerm);		if((ResultCode = ResError()) == noErr){			UseResFile(CDRemoteResFile);						if((CDRemoteIndex = (CDRemoteIndexHandle)Get1Resource(CDRemoteType,CDRemoteID)) != nil){				KeyCode = (((long)DiskTOC->DiskMaxTrack << 24) | (((long)DecimalToBCD(DiskTOC->LeadOut.Minute)) << 16) | (((long)DecimalToBCD(DiskTOC->LeadOut.Second)) << 8) | (((long)DecimalToBCD(DiskTOC->LeadOut.Frame))));				for(ID=0;ID<(*CDRemoteIndex)->MaxRecord;ID++){					if((*CDRemoteIndex)->Record[ID].KeyCode == KeyCode){						RecordID = (*CDRemoteIndex)->Record[ID].RemoteID;						GetIndString(NameTOC->DiskName,RecordID,1);						for(ID=0;ID<DiskTOC->DiskMaxTrack;ID++) GetIndString(NameTOC->TrackName[ID],RecordID,ID+2);						break;					}				}			}			else ResultCode = ResError();						UseResFile(SaveResFile);			CloseResFile(CDRemoteResFile);		}	}	return(ResultCode);}/*CDReadNameTOC*/OSErr CDReadTheQSubcode(CDQSubcodePtr DiskStatus,short Index){	CDDriveParam		CDParam;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 107;		if((ResultCode = PBControl((ParmBlkPtr)&CDParam,false)) == noErr){		DiskStatus->CurrentStatus = CDParam.Option.AudioStatus.StatusField;		DiskStatus->PlayMode = CDParam.Option.AudioStatus.PlayMode & 0x0F;		DiskStatus->ControlField = CDParam.Option.AudioStatus.ControlField & 0x0F;		DiskStatus->DiskTime.Minute = BCDToDecimal(CDParam.Option.AudioStatus.DiskMinute);		DiskStatus->DiskTime.Second = BCDToDecimal(CDParam.Option.AudioStatus.DiskSecond);		DiskStatus->DiskTime.Frame = BCDToDecimal(CDParam.Option.AudioStatus.DiskFrame);				MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));		CDParam.Default.ioCRefNum = Index;		CDParam.Default.csCode = 101;				if((ResultCode = PBControl((ParmBlkPtr)&CDParam,false)) == noErr){			DiskStatus->CurrentTrack = BCDToDecimal(CDParam.Option.ReadTheQSubcode.CurrentTrack);			DiskStatus->TrackTime.Minute = BCDToDecimal(CDParam.Option.ReadTheQSubcode.TrackMinute);			DiskStatus->TrackTime.Second = BCDToDecimal(CDParam.Option.ReadTheQSubcode.TrackSecond);			DiskStatus->TrackTime.Frame = BCDToDecimal(CDParam.Option.ReadTheQSubcode.TrackFrame);		}	}	return(ResultCode);}/*CDReadTheQSubcode*/OSErr CDAudioTrackSearch(CDDiskAddressPtr DiskTime,Boolean HoldOrStart,short Index){	CDDriveParam		CDParam;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 103;	CDParam.Option.AudioTrackSearch.AddressType = 1;	CDParam.Option.AudioTrackSearch.DiskMinute = DecimalToBCD(DiskTime->Minute);	CDParam.Option.AudioTrackSearch.DiskSecond = DecimalToBCD(DiskTime->Second);	CDParam.Option.AudioTrackSearch.DiskFrame = DecimalToBCD(DiskTime->Frame);	CDParam.Option.AudioTrackSearch.HoldOrStart = (HoldOrStart == true) ? 0 : 1;	CDParam.Option.AudioTrackSearch.PlayMode = 9;	ResultCode = PBControl((ParmBlkPtr)&CDParam,false);	return(ResultCode);}/*CDAudioTrackSearch*/OSErr CDAudioPlay(CDDiskAddressPtr StartTime,CDDiskAddressPtr StopTime,short Index){	CDDriveParam		CDParam;	CDDiskAddress		TempTime;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 104;	CDParam.Option.AudioPlay.AddressType = 1;	CDParam.Option.AudioPlay.PlayMode = 9;		if(StopTime != nil){		TempTime.Minute = 0;		TempTime.Second = 0;		TempTime.Frame = 1;		SubtractDiskAddress(StopTime,&TempTime,&TempTime);		CDParam.Option.AudioPlay.DiskMinute = DecimalToBCD(TempTime.Minute);		CDParam.Option.AudioPlay.DiskSecond = DecimalToBCD(TempTime.Second);		CDParam.Option.AudioPlay.DiskFrame = DecimalToBCD(TempTime.Frame);		CDParam.Option.AudioPlay.StartOrStop = 1;		ResultCode = PBControl((ParmBlkPtr)&CDParam,false);	}	if(StartTime != nil && ResultCode == noErr){		CDParam.Option.AudioPlay.DiskMinute = DecimalToBCD(StartTime->Minute);		CDParam.Option.AudioPlay.DiskSecond = DecimalToBCD(StartTime->Second);		CDParam.Option.AudioPlay.DiskFrame = DecimalToBCD(StartTime->Frame);		CDParam.Option.AudioPlay.StartOrStop = 0;		ResultCode = PBControl((ParmBlkPtr)&CDParam,false);	}	return(ResultCode);}/*CDAudioPlay*/OSErr CDAudioPause(Boolean PauseOrResume,short Index){	CDDriveParam		CDParam;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 105;	CDParam.Option.AudioPause.PauseOrResume = (PauseOrResume == true) ? 1 : 0;	ResultCode = PBControl((ParmBlkPtr)&CDParam,false);	return(ResultCode);}/*CDAudioPause*/OSErr CDAudioStop(short Index){	CDDriveParam		CDParam;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 106;	CDParam.Option.AudioStop.AddressType = 0;	CDParam.Option.AudioStop.DiskMinute = DecimalToBCD(0);	CDParam.Option.AudioStop.DiskSecond = DecimalToBCD(0);	CDParam.Option.AudioStop.DiskFrame = DecimalToBCD(0);	ResultCode = PBControl((ParmBlkPtr)&CDParam,false);	return(ResultCode);}/*CDAudioStop*/OSErr CDAudioScan(short DeltaTime,short Index){	CDDriveParam		CDParam;	CDDiskAddress		TempTime;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 107;		if((ResultCode = PBControl((ParmBlkPtr)&CDParam,false)) == noErr){		TempTime.Minute = CDParam.Option.AudioStatus.DiskMinute;		TempTime.Second = CDParam.Option.AudioStatus.DiskSecond;		TempTime.Frame = CDParam.Option.AudioStatus.DiskFrame;				MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));		CDParam.Default.ioCRefNum = Index;		CDParam.Default.csCode = 108;		CDParam.Option.AudioScan.AddressType = 1;		CDParam.Option.AudioScan.DiskMinute = TempTime.Minute;		CDParam.Option.AudioScan.DiskSecond = TempTime.Second;		CDParam.Option.AudioScan.DiskFrame = TempTime.Frame;		CDParam.Option.AudioScan.ForwardOrReverse = (DeltaTime > 0) ? 0 : 1;		ResultCode = PBControl((ParmBlkPtr)&CDParam,false);	}	return(ResultCode);}/*CDAudioScan*/OSErr CDAudioControl(short Left,short Right,short Index){	CDDriveParam		CDParam;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 109;	CDParam.Option.AudioControl.LeftVolume = Left;	CDParam.Option.AudioControl.RightVolume = Right;	ResultCode = PBControl((ParmBlkPtr)&CDParam,false);	return(ResultCode);}/*CDAudioControl*/OSErr CDReadAudioVolume(short *Left,short *Right,short Index){	CDDriveParam		CDParam;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 112;	if((ResultCode = PBControl((ParmBlkPtr)&CDParam,false)) == noErr){		*Left = CDParam.Option.AudioControl.LeftVolume;		*Right = CDParam.Option.AudioControl.RightVolume;	}	return(ResultCode);}/*CDReadAudioVolume*/OSErr CDGetSpindleSpeed(short *Speed,short Index){	CDDriveParam		CDParam;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 113;	if((ResultCode = PBControl((ParmBlkPtr)&CDParam,false)) == noErr) *Speed = CDParam.Option.GetSpindleSpeed.Speed;	return(ResultCode);}/*CDGetSpindleSpeed*/OSErr CDSetSpindleSpeed(short Speed,short Index){	CDDriveParam		CDParam;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 114;	CDParam.Option.SetSpindleSpeed.Speed = Speed;	ResultCode = PBControl((ParmBlkPtr)&CDParam,false);	return(ResultCode);}/*CDSetSpindleSpeed*/OSErr CDReadAudioData(CDDiskAddressPtr DiskTime,Ptr DataBlock,long Blocks,short Index){	CDDriveParam		CDParam;	OSErr				ResultCode;		ResultCode = noErr;	MemoryClear((Ptr)&CDParam,sizeof(CDDriveParam));	CDParam.Default.ioCRefNum = Index;	CDParam.Default.csCode = 115;	CDParam.Option.ReadAudio.RequestType = 0;	CDParam.Option.ReadAudio.Address = DataBlock;	CDParam.Option.ReadAudio.AddressType = 1;	CDParam.Option.ReadAudio.DiskMinute = DecimalToBCD(DiskTime->Minute);	CDParam.Option.ReadAudio.DiskSecond = DecimalToBCD(DiskTime->Second);	CDParam.Option.ReadAudio.DiskFrame = DecimalToBCD(DiskTime->Frame);	CDParam.Option.ReadAudio.RequestBlock = Blocks;	ResultCode = PBControl((ParmBlkPtr)&CDParam,false);	return(ResultCode);}/*CDReadAudioData*/static void MemoryClear(Ptr Address,long Size){	long				ID;		for(ID=0;ID<Size;ID++,Address++) *Address = 0;	return;}/*MemoryClear*/static Byte DecimalToBCD(Byte x){	return(((x / 10) << 4) + (x % 10));}/*DecimalToBCD*/static Byte BCDToDecimal(Byte x){	return(((x >> 4) * 10) + (x & 0x0F));}/*BCDToDecimal*/static void AddDiskAddress(CDDiskAddressPtr Left,CDDiskAddressPtr Right,CDDiskAddressPtr Answer){	CDDiskAddress	TempLeft;	CDDiskAddress	TempRight;		TempLeft = *Left;	TempRight = *Right;	Answer->Frame = TempLeft.Frame + TempRight.Frame;	if(Answer->Frame > 74){		Answer->Frame -= 75;		TempLeft.Second++;	}	Answer->Second = TempLeft.Second + TempRight.Second;	if(Answer->Second > 59){		Answer->Second -= 60;		TempLeft.Minute++;	}	Answer->Minute = TempLeft.Minute + TempRight.Minute;	return;}/*AddDiskAddress*/static void SubtractDiskAddress(CDDiskAddressPtr Left,CDDiskAddressPtr Right,CDDiskAddressPtr Answer){	CDDiskAddress	TempLeft;	CDDiskAddress	TempRight;		TempLeft = *Left;	TempRight = *Right;	Answer->Frame = TempLeft.Frame - TempRight.Frame;	if(Answer->Frame < 0){		Answer->Frame += 75;		TempLeft.Second--;	}	Answer->Second = TempLeft.Second - TempRight.Second;	if(Answer->Second < 0){		Answer->Second += 60;		TempLeft.Minute--;	}	Answer->Minute = TempLeft.Minute - TempRight.Minute;	return;}/*SubtractDiskAddress*/static void DiskAddressToFrame(CDDiskAddressPtr DiskTime,long *Answer){	*Answer = ((long)DiskTime->Minute * (long)60 + (long)DiskTime->Second) * (long)75 + (long)DiskTime->Frame;	return;}/*DiskAddressToFrame*/static void CopyStringByteX(unsigned char *String,unsigned char *Written){	short			ID;	short			ByteCopy;		ByteCopy = *String;	for(ID=0;ID<=ByteCopy;ID++,Written++,String++) *Written = *String;	return;}/*CopyStringByteX*/static void ConnectStringByteX(unsigned char *String,unsigned char *Written){	short			ID;		for(ID=Written[0];ID<String[0]+Written[0];ID++) Written[ID+1] = String[ID-Written[0]+1];	Written[0] = ID;	return;}/*ConnectStringByteX*/