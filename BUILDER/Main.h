#ifndef MAIN_H
#define MAIN_H
//------------------------------------------------------------------------------
#define LINUX   0
#define SHOW    0
#include <stdio.h>
#include "DCUClasses.h"
//------------------------------------------------------------------------------
#define     OutLog1(a)          if (fLog) fprintf(fLog, (a))
#define     OutLog2(a, b)       if (fLog) fprintf(fLog, (a), (b))
#define     OutLog3(a, b, c)    if (fLog) fprintf(fLog, (a), (b), (c))
#define     lfauxPropField      0x80000000
//#define DUMP_BEG    0x80
//#define FIXUP_BEG   0x81
//------------------------------------------------------------------------------
int __fastcall          AddAddrDef(TDCURec* ND);
void __fastcall         AddTypeDef(TTypeDef* TD);
void __fastcall         AddTypeName(int hDef, int hDecl, PName Name);
String __fastcall       CharStr(char Ch);
BYTE __fastcall         FixTag(BYTE Tag);
void __fastcall         RegisterEmbeddedTypes(TNameDecl* Embedded, int Depth);
void _fastcall          BindEmbeddedType(PDCURec UseRec, int hDT, DWORD* IP);
void __fastcall         BindEmbeddedTypes();
void __fastcall         EnumUsedTypeList(PDCURec L, TTypeUseAction Action, DWORD* IP);
void __fastcall         FreeDCURecList(TDCURec* L);
TDCURec* __fastcall     GetAddrDef(int hDef);
String __fastcall       GetAddrStr(int hDef);
BYTE* __fastcall        GetBlockMem(DWORD BlOfs, DWORD BlSz, DWORD* ResSz);
String __fastcall       GetDCURecStr(TDCURec* D, int hDef);
TDCURec* __fastcall     GetGlobalAddrDef(int hDef);
TTypeDef* __fastcall    GetLocalTypeDef(int hDef);
TTypeDef* __fastcall    GetGlobalTypeDef(int hDef);
int __fastcall          GetStartFixup(DWORD Ofs);
TTypeDef* __fastcall    GetTypeDef(int hDef);
int __fastcall          GetTypeSize(int hDef);
PUnitImpRec __fastcall  GetUnitImpRec(int hUnit);
bool __fastcall         MemToUInt(BYTE *DP, DWORD Sz, DWORD* Res);
String __fastcall       NDXToStr(int NDXLo);
String __fastcall       PName2String(PName Name);
BYTE __fastcall         ReadByte();
void __fastcall         ReadByteIfEQ(BYTE V);
int __fastcall          ReadByteFrom(bool b);
BYTE __fastcall         ReadCallKind();
int __fastcall          ReadClassInterfaces(int** PITbl);
int __fastcall          ReadConstAddInfo(TNameDecl* LastProcDecl);
void __fastcall         ReadDeclList(BYTE LK, TNameDecl** Result);
int __fastcall          ReadIndex();
void __fastcall         ReadIndex64(PInt64Rec Res);
BYTE* __fastcall        ReadMem(DWORD Sz);
PName __fastcall        ReadName();
BYTE __fastcall         ReadTag();
int __fastcall          ReadUIndex();
DWORD __fastcall        ReadULong();
void __fastcall         RefAddrDef(int V);
bool __fastcall         RegTypeShow(TBaseDef* T);
void __fastcall         RestoreFixupMemState(TFixupMemState* S);
void __fastcall         SaveFixupMemState(TFixupMemState* S);
void __fastcall         SetCodeRange(BYTE* ACodeStart, BYTE* ACodeBase, DWORD ABlSz);
void __fastcall         SetProcAddInfo(int V);
void __fastcall         ShowDataBl(DWORD Ofs0, DWORD BlOfs, DWORD BlSz);
void __fastcall         ShowCodeBl(DWORD Ofs0, DWORD BlOfs, DWORD BlSz);
void __fastcall         ShowDeclList(BYTE LK, TNameDecl* Decl, String& OutS);
void __fastcall         ShowDump(BYTE* DP, BYTE* DPFile0, DWORD FileSize, DWORD SizeDispl, DWORD Size, DWORD Ofs0Displ, DWORD Ofs0, DWORD WMin, int FixCnt, TFixupRec* FixTbl);
bool __fastcall         ShowGlobalConstValue(int hDef, String& OutS);
String __fastcall       ShowOfsQualifier(int hDef, int Ofs);
int __fastcall          ShowGlobalTypeValue(int hDef, BYTE* DP, DWORD DS, bool AndRest, int ConstKind, String& OutS);
int __fastcall          ShowStrConst(BYTE* DP, DWORD DS, String& OutS);
int __fastcall          ShowUnicodeStrConst(BYTE* DP, DWORD DS, String& OutS); //Ver >=verD12
int __fastcall          ShowUnicodeResStrConst(BYTE* DP, DWORD DS, String& OutS); //Ver >=verD12
String __fastcall       ShowRefOfsQualifier(int hDef, int Ofs);
String __fastcall       ShowTypeDef(int hDef, PName N);
String __fastcall       ShowTypeName(int hDef);
int __fastcall          ShowTypeValue(TTypeDef* T, BYTE* DP, DWORD DS, int ConstKind, String& OutS);
void __fastcall         SkipBlock(int Sz);
String __fastcall       StrConstStr(char* CP, int L);
bool __fastcall         TypeIsVoid(int hDef);
void __fastcall         UnRegTypeShow(TBaseDef* T);
//------------------------------------------------------------------------------
//OffsetsInfo
typedef struct
{
    DWORD       Offset;
    DWORD       Size;
    int         ModId;    //Modules
    int         NamId;    //Names
} OFFSETSINFO, *POFFSETSINFO;
//Module info
typedef struct
{
    int         ID;
    WORD        ModuleID;
    DWORD       Offset;
    DWORD       Size;
    String      Name;           //Unit Name
    String      Filename;       //Unit Filename
    TStringList *UsesList;      //List of Uses
} MODULEINFO, *PMODULEINFO;
//Fixup info
typedef struct
{
    BYTE        Type;           //A-ADR;J-JMP;D-DAT
    DWORD       Ofs;            //Offset from RTTI data begin
    String      Name;           //Name
} FIXUPINFO, *PFIXUPINFO;
//ConstInfo
#define CI_CONSTDECL    'C'
#define CI_PDECL        'P'
#define CI_VARCDECL     'V'
typedef struct
{
    int         ID;
    DWORD       Offset;
    DWORD       Size;
    bool        Skip;
    WORD        ModuleID;
    String      Name;
    BYTE        Type;           //look above
    String      TypeDef;
    String      Value;
    DWORD       RTTISz;         //Size of RTTI data
    DWORD       RTTIOfs;        //Offset of RTTI data
    TList       *Fixups;        //If VMT
} CONSTINFO, *PCONSTINFO;
//TypeInfo
typedef struct
{
    int         ID;
    DWORD       Offset;
    DWORD       Size;
    WORD        ModuleID;
    String      Name;
    BYTE        Kind;
    WORD        VMCnt;          //Number of class VM
    DWORD       RTTISz;         //Size of RTTI data
    DWORD       RTTIOfs;        //Offset of RTTI data
    String      Decl;
    TList       *Fixups;
    TList       *Fields;        //List of Fields
    TList       *Properties;    //List of Properties
    TList       *Methods;       //List of Methods
} TYPEINFO, *PTYPEINFO;
//VarInfo
#define VI_VAR          'V'
#define VI_ABSVAR       'A'
#define VI_SPECVAR      'S'
#define VI_THREADVAR    'T'
typedef struct
{
    int         ID;
    DWORD       Offset;
    DWORD       Size;
    WORD        ModuleID;
    String      Name;
    BYTE        Type;           //look above
    DWORD       DumpOfs;        //Offset of binary data
    DWORD       DumpSz;         //Size of binary data
    String      AbsName;
    String      TypeDef;
} VARINFO, *PVARINFO;
//ResourseStringInfo
typedef struct
{
    int         ID;
    DWORD       Offset;
    DWORD       Size;
    WORD        ModuleID;
    String      Name;
    DWORD       DumpOfs;        //Offset of binary data
    DWORD       DumpSz;         //Size of binary data
    String      TypeDef;
    String      Context;        //Context of ResStr
} RESSTRINFO, *PRESSTRINFO;
//LocalDeclInfo
typedef struct
{
    BYTE        Scope;
    BYTE        Tag;
    int         LocFlags;
    int         Ndx;
    int         NdxB;
    int         Case;           //for case
    String      Name;
    String      TypeDef;
    String      AbsName;
} LOCALDECLINFO, *PLOCALDECLINFO;
//PropertyInfo
typedef struct
{
    BYTE        Scope;
    int         Index;
    int         DispId;
    String      Name;
    String      TypeDef;
    String      ReadName;
    String      WriteName;
    String      StoredName;
} PROPERTYINFO, *PPROPERTYINFO;
//MethodDeclInfo
typedef struct
{
    BYTE        Scope;
    BYTE        MethodKind;     //'M'-method,'P'-procedure,'F'-function,'C'-constructor,'D'-destructor
    String      Prototype;
} METHODDECLINFO, *PMETHODDECLINFO;
//------------------------------------------------------------------------------
#endif
/*
TDCURec [Next]
    *TNameDecl [Def, hDecl]
        *TNameFDecl [F, Inf, B2]
            *TTypeDecl [hDef]
            *TVarDecl [hDT, Ofs]
                *TVarCDecl [Sz, OfsR]
                    *TTypePDecl
                    *TResStrDef [OfsR]
                TAbsVarDecl
                TThreadVarDecl
                TSpecVar
            *TStrConstDecl [hDT, Ofs, Sz]
            *TConstDeclBase [hDT, hX, ValPtr, ValSz, Val]
                *TConstDecl
            *TProcDecl [CodeOfs, AddrBase, Sz, B0, VProc, hDTRes, Args, Locals, Embedded, CallKind, MethodKind, JustData, FProcLocVarTbl, FProcLocVarCnt]
                *TSysProc8Decl [F, Ndx]
            *TUnitAddInfo [B, Sub]
        *TLabelDecl [Ofs]
        *TExportDecl [hSym, Index]
        *TLocalDecl [LocFlags, hDT, NdxB, Ndx]
            *TMethodDecl [InIntrf, hImport]
            TClassVarDecl
            TDispPropDecl
        *TPropDecl [LocFlags, hDT, Ndx, hIndex, hRead, hWrite, hStored, hDeft]
        *TSetDeftInfo [hConst, hArg]
        *TCopyDecl [hBase, Base]
        *TSysProcDecl [F, Ndx]
    *TBaseDef [FName, Def, hUnit]
        *TImpDef [ik, FNameIsUnique, Inf]
        *TDLLImpRec [Ndx]
        *TImpTypeDefRec [RTTIOfs, RTTISz, hImpUnit, ImpName]
        *TTypeDef [RTTISz, Sz, V, RTTIOfs]
            TRangeBaseDef [hDTBase, LH, B]
                *TRangeDef
                *TEnumDef [Ndx, NameTbl]
            *TFloatDef [B]
            *TPtrDef [hRefDT]
            TTextDef
            *TFileDef [hBaseDT]
            *TSetDef [BStart, hBaseDT]
            *TArrayDef [B1, hDTNdx, hDTEl]
                TShortStrDef
                TStringDef
            *TVariantDef [B]
            *TObjVMTDef [hObjDT, Ndx1]
            TRecBaseDef [Fields]
                *TRecDef [B2]
                *TProcTypeDef [Ndx0, hDTRes, AddStart, AddSz, CallKind]
                *TObjDef [B03, hParent, BFE, Ndx1, B00]
                *TClassDef [hParent, InstBaseRTTISz, InstBaseSz, InstBaseV, VMCnt, NdxFE, Ndx00a, B04, ICnt, ITbl]
                    TMetaClassDef [hCl]
                *TInterfaceDef [hParent, VMCnt, GUID, B]
            TVoidDef
*/

