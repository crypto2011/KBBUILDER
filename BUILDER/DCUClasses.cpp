#include <vcl.h>
#include "DCUClasses.h"
#include "main.h"
#include <typeinfo.h>
#include <values.h>
//------------------------------------------------------------------------------
extern DWORD        *pDumpOffset;
extern DWORD        *pDumpSize;
extern TList        *FixupsList;
extern BYTE         ActiveScope;
extern TList        *FieldsList;
extern TList        *PropertiesList;
extern TList        *MethodsList;
extern TList        *ProcsList;
extern TList        *ArgsList;
extern TList        *LocalsList;
extern FILE         *fOut;
extern bool         ThreadVar;
extern WORD         ModuleID;
extern TList        *ConstList;
extern TList        *TypeList;
extern TList        *VarList;
extern TList        *ResStrList;
extern TList        *ProcList;
extern int          CaseN;

extern bool         GenVarCAsVars;
extern int          FVer;
extern bool         IsMSIL;
extern int          NDXHi;
extern BYTE         *DefStart;
extern BYTE         *CurPos;
extern BYTE         Tag;
extern int          FTypeDefCnt;
extern BYTE         *FDataBlPtr;
extern int          FFixupCnt;
extern TFixupRec    *FFixupTbl;
extern BYTE         fxStart; 
extern TShortString NoName;
extern FILE         *fLog;
//------------------------------------------------------------------------------
__fastcall TDCURec::TDCURec() : TObject()
{
    Next = 0;
}
//------------------------------------------------------------------------------
PName __fastcall TDCURec::GetName()
{
    return NULL;
}
//------------------------------------------------------------------------------
DWORD __fastcall TDCURec::SetMem(DWORD MOfs, DWORD MSz)
{
    return 0;
}
//------------------------------------------------------------------------------
bool __fastcall TDCURec::NameIsUnique()
{
    return false;
}
//------------------------------------------------------------------------------
void __fastcall TDCURec::ShowName(String& OutS)
{
    OutS = "";
}
//------------------------------------------------------------------------------
void __fastcall TDCURec::Show(String& OutS)
{
    OutS = "";
}
//------------------------------------------------------------------------------
//for verD_XE - fix orphaned local types problem
void __fastcall TDCURec::EnumUsedTypes(TTypeUseAction Action, DWORD* IP)
{
//no type used
}
//------------------------------------------------------------------------------
__fastcall TBaseDef::TBaseDef(PName AName, PNameDef ADef, int AUnit):TDCURec()
{
    FName = AName;
    Def = ADef;
    hUnit = AUnit;
}
//------------------------------------------------------------------------------
void __fastcall TBaseDef::ShowName(String& OutS)
{
    PUnitImpRec U;
    PName NP;

    NP = FName;
    if (!NP || !NP->Len) NP = &NoName;
    if (hUnit < 0)
    {
        if (NP->Len)
        {
            OutS = GetDCURecStr(this, hDecl);
            OutLog2("%s", OutS.c_str());
        }
    }
    else if (NameIsUnique())
    {
        OutS = PName2String(NP);
        OutLog2("%s", OutS.c_str());
    }
    else
    {
        U = GetUnitImpRec(hUnit);
        OutS = PName2String(U->Name) + "." + PName2String(NP);
        OutLog3("%s.%s", PName2String(U->Name).c_str(), PName2String(NP).c_str());
    }
}
//------------------------------------------------------------------------------
void __fastcall TBaseDef::Show(String& OutS)
{
    PName NP;

    NP = FName;
    if (!NP || !NP->Len) NP = &NoName;
    OutS = PName2String(NP);
    OutLog2("%s", OutS.c_str());
}
//------------------------------------------------------------------------------
void __fastcall TBaseDef::ShowNamed(PName N, String& OutS)
{
    if ((N && N == FName || !FName || !FName->Len) && RegTypeShow(this))
    {
        __try
        {
            Show(OutS);
        }
        catch(...)
        {
            UnRegTypeShow(this);
        }
    }
    else
        ShowName(OutS);
}
//------------------------------------------------------------------------------
PName __fastcall TBaseDef::GetName()
{
    if (!FName) return &NoName;
    else return FName;
}
//------------------------------------------------------------------------------
DWORD __fastcall TBaseDef::SetMem(DWORD MOfs, DWORD MSz)
{
    return 0;
}
//------------------------------------------------------------------------------
__fastcall TImpDef::TImpDef(BYTE AIK, PName AName, int AnInf, PNameDef ADef, int AUnit):
    TBaseDef(AName, ADef, AUnit)
{
    Inf = AnInf;
    ik = AIK;
}
//------------------------------------------------------------------------------
void __fastcall TImpDef::Show(String& OutS)
{
    String S;
    OutS = String(ik) + ":";
    OutLog2("%s:", OutS.c_str());
    TBaseDef::Show(S);
    OutS += S;
}
//------------------------------------------------------------------------------
bool __fastcall TImpDef::NameIsUnique()
{
    return FNameIsUnique;
}
//------------------------------------------------------------------------------
__fastcall TDLLImpRec::TDLLImpRec(PName AName, int ANdx, PNameDef ADef, int AUnit):
    TBaseDef(AName, ADef, AUnit)
{
    Ndx = ANdx;
}
//------------------------------------------------------------------------------
void __fastcall TDLLImpRec::Show(String& OutS)
{
    OutS = "";
    bool NoName = (!FName || !FName->Len);
    String Name;
    if (!NoName)
    {
        Name = PName2String(FName);
        OutS += "name '" + Name + "'";
        OutLog2("name '%s'", Name.c_str());
    }
    if (NoName || Ndx)
    {
        OutS += "index " + IntToHex(Ndx, 0);
        OutLog2("index %lX", Ndx);
    }
}
//------------------------------------------------------------------------------
__fastcall TImpTypeDefRec::TImpTypeDefRec(PName AName, int AnInf, DWORD ARTTISz, PNameDef ADef, int AUnit):
    TImpDef('T', AName, AnInf, ADef, AUnit)
{
    RTTISz = ARTTISz;
    RTTIOfs = -1;
    hImpUnit = hUnit;
    hUnit = -1;
    ImpName = FName;
    FName = NULL;   //Will be named later in the corresponding TTypeDecl
}
//------------------------------------------------------------------------------
void __fastcall TImpTypeDefRec::Show(String& OutS)
{
    PUnitImpRec U;
    String Name;

    OutS = "type ";
    OutLog1("type ");
    if (hImpUnit >= 0)
    {
        U = GetUnitImpRec(hImpUnit);
        Name = PName2String(U->Name);
        OutS += Name + ".";
        OutLog2("%s.", Name.c_str());
    }
    Name = PName2String(ImpName);
    OutS += Name;
    OutLog2("%s", Name.c_str());
    if (RTTISz > 0)
    {
        OutLog1("RTTI: ");
        ShowDataBl(0, RTTIOfs, RTTISz);
    }
}
//------------------------------------------------------------------------------
DWORD __fastcall TImpTypeDefRec::SetMem(DWORD MOfs, DWORD MSz)
{
    RTTIOfs = MOfs;
    return 0;
}
//------------------------------------------------------------------------------
__fastcall TNameDecl::TNameDecl():TDCURec()
{
}
//------------------------------------------------------------------------------
__fastcall TNameDecl::TNameDecl(bool All):TDCURec()
{
    PName N;

    hDecl = AddAddrDef(this);
    if (All)
    {
        Def = (PNameDef)DefStart;
        N = ReadName();
//!!!
if (strlen("TAction") == N->Len && !memcmp(N->Name, "TAction", strlen("TAction")))
N = N;
    }
};
//------------------------------------------------------------------------------
__fastcall TNameDecl::~TNameDecl()
{
}
//------------------------------------------------------------------------------
void __fastcall TNameDecl::ShowName(String& OutS)
{
    OutS = GetDCURecStr(this, hDecl);
    OutLog2("%s", OutS.c_str());
    if (SameText(OutS, "TAction"))
        OutS = "TAction";
}
//------------------------------------------------------------------------------
void __fastcall TNameDecl::Show(String& OutS)
{
    ShowName(OutS);
}
//------------------------------------------------------------------------------
void __fastcall TNameDecl::ShowDef(bool All, String& OutS)
{
    Show(OutS);
}
//------------------------------------------------------------------------------
PName __fastcall TNameDecl::GetName()
{
    if (!Def) return &NoName;
    else return &Def->Name;
}
//------------------------------------------------------------------------------
DWORD __fastcall TNameDecl::SetMem(DWORD MOfs, DWORD MSz)
{
    return 0;
}
//------------------------------------------------------------------------------
BYTE __fastcall TNameDecl::GetTag()
{
    return FixTag(Def->Tag);
}
//------------------------------------------------------------------------------
BYTE __fastcall TNameDecl::GetSecKind()
{
    return skNone;
}
//------------------------------------------------------------------------------
bool __fastcall TNameDecl::IsVisible(BYTE LK)
{
    return true;
}
//------------------------------------------------------------------------------
__fastcall TNameFDecl::TNameFDecl(bool NoInf):TNameDecl(true)
{
    int F3, F4;

    F = ReadUIndex();
    if (FVer >= verD8 && FVer < verK1) F1 = ReadUIndex();
    if (FVer >= verD2009 && FVer < verK1) F4 = ReadUIndex();
    if (!NoInf && ((F & 0x40) != 0))
        Inf = ReadULong();
    if (FVer >= verD8 && FVer < verK1)
    {
        if ((F1 & 0x80) != 0)   //Could be valid for MSIL only
        {
            B2 = ReadUIndex();
            if (FVer == verD8 && ((F & 8) != 0))
                F3 = ReadUIndex();
        }
    }
};
//------------------------------------------------------------------------------
void __fastcall TNameFDecl::Show(String& OutS)
{
    TNameDecl::Show(OutS);
}
//------------------------------------------------------------------------------
bool __fastcall TNameFDecl::IsVisible(BYTE LK)
{
    if (LK == dlMain) return ((F & 0x40) != 0);
    if (LK == dlMainImpl) return ((F & 0x40) == 0);
    return true;
}
//------------------------------------------------------------------------------
__fastcall TTypeDecl::TTypeDecl():TNameFDecl(false)
{
    hDef = ReadUIndex();
    if (FVer >= verD8 && FVer < verK1 && B2)
        hDef = B2;
    AddTypeName(hDef, hDecl, &Def->Name);
}
//------------------------------------------------------------------------------
bool __fastcall TTypeDecl::IsVisible(BYTE LK)
{
    return TNameFDecl::IsVisible(LK);
}
//------------------------------------------------------------------------------
int M = 0;
void __fastcall TTypeDecl::Show(String& OutS)
{
    String S;
    PName RefName;

    OutS = "";
    PTYPEINFO typeInfo = new TYPEINFO;
    typeInfo->Size = GetTypeSize(hDef);
    typeInfo->ModuleID = ModuleID;
    typeInfo->Kind = 'Z';  //drAlias
    typeInfo->VMCnt = 0;
    typeInfo->RTTIOfs = 0xFFFFFFFF;
    typeInfo->RTTISz = 0;
    typeInfo->Fixups = new TList;
    typeInfo->Fields = new TList;
    typeInfo->Properties = new TList;
    typeInfo->Methods = new TList;
    TNameFDecl::Show(S);
    typeInfo->Name = S;
    OutS += S;
    if (!Def)
        RefName = NULL;
    else
        RefName = &Def->Name;
    OutS += "=";
    OutLog1("=");
    TBaseDef *D = GetTypeDef(hDef);
    if (D)
    {
        if (D->InheritsFrom((__classid(TArrayDef))))
            typeInfo->Kind = drArrayDef;
        else if (D->InheritsFrom((__classid(TClassDef))))
        {
            typeInfo->Kind = drClassDef;
            typeInfo->VMCnt = ((TClassDef*)D)->VMCnt;
            FieldsList = typeInfo->Fields;
            PropertiesList = typeInfo->Properties;
            MethodsList = typeInfo->Methods;
        }
        else if (D->InheritsFrom((__classid(TFileDef))))
            typeInfo->Kind = drFileDef;
        else if (D->InheritsFrom((__classid(TFloatDef))))
            typeInfo->Kind = drFloatDef;
        else if (D->InheritsFrom((__classid(TInterfaceDef))))
        {
            typeInfo->Kind = drInterfaceDef;
            typeInfo->VMCnt = ((TInterfaceDef*)D)->VMCnt;
            FieldsList = typeInfo->Fields;
            PropertiesList = typeInfo->Properties;
            MethodsList = typeInfo->Methods;
        }
        else if (D->InheritsFrom((__classid(TObjVMTDef))))
            typeInfo->Kind = drObjVMTDef;
        else if (D->InheritsFrom((__classid(TProcTypeDef))))
            typeInfo->Kind = drProcTypeDef;
        else if (D->InheritsFrom((__classid(TPtrDef))))
            typeInfo->Kind = drPtrDef;
        else if (D->InheritsFrom((__classid(TRangeBaseDef))))
            typeInfo->Kind = drRangeDef;
        else if (D->InheritsFrom((__classid(TRecDef))))
        {
            typeInfo->Kind = drRecDef;
            FieldsList = typeInfo->Fields;
        }
        else if (D->InheritsFrom((__classid(TSetDef))))
            typeInfo->Kind = drSetDef;
        else if (D->InheritsFrom((__classid(TShortStrDef))))
            typeInfo->Kind = drShortStrDef;
        else if (D->InheritsFrom((__classid(TStringDef))))
            typeInfo->Kind = drStringDef;
        else if (D->InheritsFrom((__classid(TTextDef))))
            typeInfo->Kind = drTextDef;
        else if (D->InheritsFrom((__classid(TVariantDef))))
            typeInfo->Kind = drVariantDef;
    }
    
    pDumpOffset = &typeInfo->RTTIOfs;
    pDumpSize = &typeInfo->RTTISz;
    FixupsList = typeInfo->Fixups;
    S = ShowTypeDef(hDef, RefName);
    pDumpOffset = 0;
    pDumpSize = 0;
    FixupsList = 0;
    FieldsList = 0;
    PropertiesList = 0;
    MethodsList = 0;

    typeInfo->Decl = S;
    OutS += S;
    if (TypeList)
        TypeList->Add((void*)typeInfo);
    else
    {
        delete typeInfo->Fixups;
        delete typeInfo->Fields;
        delete typeInfo->Properties;
        delete typeInfo->Methods;
        delete typeInfo;
    }
}
//------------------------------------------------------------------------------
void __fastcall TTypeDecl::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    TBaseDef    *D;

    D = GetTypeDef(hDef);
    if (!D || !(D->InheritsFrom(__classid(TTypeDef)))) return;
    if (((TTypeDef*)D)->hAddrDef != hDecl)
        Action(this, hDef, IP);
    else
        D->EnumUsedTypes(Action, IP);
}
//------------------------------------------------------------------------------
PName __fastcall TTypeDecl::GetName()
{
    TTypeDef    *TD;

    if (FVer >= verD2009 && FVer < verK1) //The template name could be fixed
    {
        TD = GetLocalTypeDef(hDef);
        if (TD && TD->hDecl == hDecl) return TD->FName;
    }
    return TNameDecl::GetName();
}
//------------------------------------------------------------------------------
DWORD __fastcall TTypeDecl::SetMem(DWORD MOfs, DWORD MSz)
{
    TTypeDef *D = GetTypeDef(hDef);
    if (!D) return 0;
    return D->SetMem(MOfs, MSz);
}
//------------------------------------------------------------------------------
BYTE __fastcall TTypeDecl::GetSecKind()
{
    return skType;
}
//------------------------------------------------------------------------------
__fastcall TVarDecl::TVarDecl():TNameFDecl(false)
{
    hDT = ReadUIndex();
    Ofs = ReadUIndex();
}
//------------------------------------------------------------------------------
void __fastcall TVarDecl::Show(String& OutS)
{
    String S;

    PVARINFO vInfo = new VARINFO;
    vInfo->ModuleID = ModuleID;
    vInfo->Type = VI_VAR;
    if (ThreadVar) vInfo->Type = VI_THREADVAR;
    vInfo->DumpOfs = 0xFFFFFFFF;
    vInfo->DumpSz = 0;
    vInfo->AbsName = "";

    TNameFDecl::Show(S);
    vInfo->Name = S;
    OutS = S + ":";
    OutLog1(":");
    pDumpOffset = &vInfo->DumpOfs;
    pDumpSize = &vInfo->DumpSz;
    S = ShowTypeDef(hDT, NULL);
    pDumpOffset = 0;
    pDumpSize = 0;

    vInfo->TypeDef = S;
    OutS += S;
    VarList->Add((void*)vInfo);
}
//------------------------------------------------------------------------------
void __fastcall TVarDecl::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    Action(this, hDT, IP);
}
//------------------------------------------------------------------------------
BYTE __fastcall TVarDecl::GetSecKind()
{
    return skVar;
}
//------------------------------------------------------------------------------
__fastcall TVarCDecl::TVarCDecl(bool OfsValid):TVarDecl()
{
    Sz = -1;
    OfsR = Ofs;
    if (!OfsValid) Ofs = -1;
}
//------------------------------------------------------------------------------
int         CodeFixupCnt;
PFixupRec   CodeFixups;
BYTE        *CodeBase;
BYTE        *CodeEnd;
BYTE        *CodeStart;
BYTE        *FixUpEnd;

void __fastcall ClearFixupInfo()
{
    CodeFixupCnt = 0;
    CodeFixups = NULL;
}
//------------------------------------------------------------------------------
void __fastcall SetFixupInfo(int ACodeFixupCnt, PFixupRec ACodeFixups)
{
    CodeFixupCnt = ACodeFixupCnt;
    CodeFixups = ACodeFixups;
}
//------------------------------------------------------------------------------
void __fastcall SetCodeRange(BYTE* ACodeStart, BYTE* ACodeBase, DWORD ABlSz)
{
    ClearFixupInfo();
    CodeStart = ACodeStart;
    CodeBase = ACodeBase;
    CodeEnd = CodeBase + ABlSz;
    FixUpEnd = CodeBase;
}
//------------------------------------------------------------------------------
void __fastcall SaveFixupState(TFixupState* S)
{
    S->FixCnt = CodeFixupCnt;
    S->Fix = CodeFixups;
    S->FixEnd = FixUpEnd;
}
//------------------------------------------------------------------------------
void __fastcall RestoreFixupState(TFixupState* S)
{
    CodeFixupCnt = S->FixCnt;
    CodeFixups = S->Fix;
    FixUpEnd = S->FixEnd;
}
//------------------------------------------------------------------------------
void __fastcall SaveFixupMemState(TFixupMemState* S)
{
    SaveFixupState(&S->Fx);
    S->CodeBase = CodeBase;
    S->CodeEnd = CodeEnd;
    S->CodeStart = CodeStart;
}
//------------------------------------------------------------------------------
void __fastcall RestoreFixupMemState(TFixupMemState* S)
{
    RestoreFixupState(&S->Fx);
    CodeBase = S->CodeBase;
    CodeEnd = S->CodeEnd;
    CodeStart = S->CodeStart;
}
//------------------------------------------------------------------------------
void __fastcall SetStartFixupInfo(int Fix0)
{
    SetFixupInfo(FFixupCnt - Fix0, &FFixupTbl[Fix0]);
}
//------------------------------------------------------------------------------
void __fastcall TVarCDecl::Show(String& OutS)
{
    BYTE *DP;
    DWORD DS;
    int Fix0;
    TFixupMemState MS;
    String S;

    PCONSTINFO constInfo = new CONSTINFO;
    constInfo->ModuleID = ModuleID;
    constInfo->Type = CI_VARCDECL;
    constInfo->TypeDef = "";
    constInfo->Value = "";
    constInfo->RTTIOfs = 0xFFFFFFFF;
    constInfo->RTTISz = 0;
    constInfo->Fixups = new TList;

    pDumpOffset = &constInfo->RTTIOfs;
    pDumpSize = &constInfo->RTTISz;
    FixupsList = constInfo->Fixups;

    TNameFDecl::Show(S);
    constInfo->Name = S;
    OutLog1(":");
    constInfo->TypeDef = ShowTypeDef(hDT, NULL);

    OutLog1("=");
    if (Sz == (DWORD)-1)
    {
        constInfo->Value = "?";
        OutLog1("?");
    }
    else
    {
        DP = GetBlockMem(Ofs, Sz, &DS);
        if (DP)
        {
            SaveFixupMemState(&MS);
            SetCodeRange(FDataBlPtr, DP, DS);
            Fix0 = GetStartFixup(Ofs);
            SetStartFixupInfo(Fix0);
        }
        ShowGlobalTypeValue(hDT, DP, DS, true, -1, S);
        constInfo->Value = S;
        if (DP) RestoreFixupMemState(&MS);
    }
    pDumpOffset = 0;
    pDumpSize = 0;
    FixupsList = 0;
    if (ConstList)
        ConstList->Add((void*)constInfo);
    else
    {
        delete constInfo->Fixups;
        delete constInfo;
    }
    OutS = "";
}
//------------------------------------------------------------------------------
DWORD __fastcall TVarCDecl::SetMem(DWORD MOfs, DWORD MSz)
{
    if (Sz == (DWORD)-1) Sz = MSz;
    if (Ofs == (DWORD)-1) Ofs = MOfs;
    return 0;
}
//------------------------------------------------------------------------------
BYTE __fastcall TVarCDecl::GetSecKind()
{
    if (GenVarCAsVars)
        return skVar;
    else
        return skConst;
}
//------------------------------------------------------------------------------
__fastcall TAbsVarDecl::TAbsVarDecl():TVarDecl()
{
    RefAddrDef(Ofs); //forward references could happen e.g. by referencing Self in embedded proc
}
//------------------------------------------------------------------------------
void __fastcall TAbsVarDecl::Show(String& OutS)
{
    String S;

    PVARINFO vInfo = new VARINFO;
    vInfo->ModuleID = ModuleID;
    vInfo->Type = VI_ABSVAR;
    vInfo->DumpOfs = 0xFFFFFFFF;
    vInfo->DumpSz = 0;

    TNameFDecl::Show(S);
    vInfo->Name = S;
    OutS = S + ":";
    OutLog1(":");
    pDumpOffset = &vInfo->DumpOfs;
    pDumpSize = &vInfo->DumpSz;
    S = ShowTypeDef(hDT, NULL);
    pDumpOffset = 0;
    pDumpSize = 0;

    vInfo->TypeDef = S;
    OutS += S;

    S = GetAddrStr((int)Ofs);
    vInfo->AbsName = S;
    OutS += " absolute " + S;
    OutLog2(" absolute %s", S.c_str());
    
    VarList->Add((void*)vInfo);
}
//------------------------------------------------------------------------------
__fastcall TTypePDecl::TTypePDecl() : TVarCDecl(false)
{
}
//------------------------------------------------------------------------------
void __fastcall TTypePDecl::Show(String& OutS)
{
    BYTE *DP;
    DWORD DS;
    int Fix0;
    TFixupMemState MS;
    String S;

    PCONSTINFO constInfo = new CONSTINFO;
    constInfo->ModuleID = ModuleID;
    constInfo->Type = CI_PDECL;
    constInfo->TypeDef = "";
    constInfo->Value = "";
    constInfo->RTTIOfs = 0xFFFFFFFF;
    constInfo->RTTISz = 0;
    constInfo->Fixups = new TList;

    OutLog1("VMT ");
    pDumpOffset = &constInfo->RTTIOfs;
    pDumpSize = &constInfo->RTTISz;
    FixupsList = constInfo->Fixups;

    TNameFDecl::Show(S);
    constInfo->Name = S;
    OutLog1(":");
    constInfo->TypeDef = ShowTypeDef(hDT, NULL);

    OutLog1("=");
    if (Sz == (DWORD)-1)
    {
        constInfo->Value = "?";
        OutLog1("?");
    }
    else
    {
        DP = GetBlockMem(Ofs, Sz, &DS);
        if (DP)
        {
            SaveFixupMemState(&MS);
            SetCodeRange(FDataBlPtr, DP, DS);
            Fix0 = GetStartFixup(Ofs);
            SetStartFixupInfo(Fix0);
        }
        ShowGlobalTypeValue(hDT, DP, DS, true, -1, S);
        constInfo->Value = S;
        if (DP) RestoreFixupMemState(&MS);
    }

    pDumpOffset = 0;
    pDumpSize = 0;
    FixupsList = 0;
    if (ConstList)
        ConstList->Add((void*)constInfo);
    else
    {
        delete constInfo->Fixups;
        delete constInfo;
    }
    OutS = "";
}
//------------------------------------------------------------------------------
bool __fastcall TTypePDecl::IsVisible(BYTE LK)
{
    return true;
}
//------------------------------------------------------------------------------
__fastcall TThreadVarDecl::TThreadVarDecl():TVarDecl()
{
}
//------------------------------------------------------------------------------
BYTE __fastcall TThreadVarDecl::GetSecKind()
{
    return skThreadVar;
}
//------------------------------------------------------------------------------
__fastcall TStrConstDecl::TStrConstDecl():TNameFDecl(false)
{
    Sz = ReadUIndex();
    hDT = ReadUIndex();
    if (FVer >= verDXE1 && FVer < verK1) int X = ReadUIndex();
    if (!Sz) Sz = (DWORD)-1;
    Ofs = (DWORD)-1;
}
//------------------------------------------------------------------------------
DWORD __fastcall TStrConstDecl::SetMem(DWORD MOfs, DWORD MSz)
{
    if (Sz == (DWORD)-1) Sz = MSz;
    if (Ofs == (DWORD)-1) Ofs = MOfs;
    return 0;
}
//------------------------------------------------------------------------------
BYTE __fastcall TStrConstDecl::GetSecKind()
{
    if (GenVarCAsVars)
        return skVar;
    else
        return skConst;
}
//------------------------------------------------------------------------------
void __fastcall TStrConstDecl::Show(String& OutS)
{
    BYTE *DP;
    DWORD DS;
    int Fix0;
    TFixupMemState MS;
    String S;

    TNameFDecl::Show(OutS);
    OutS += ":";
    OutLog1(":");
    OutS += ShowTypeDef(hDT, NULL);
    OutLog1("=");
    if (Sz == (DWORD)-1)
    {
        OutLog1("?");
    }
    else
    {
        DP = GetBlockMem(Ofs, Sz, &DS);
        if (DP)
        {
            SaveFixupMemState(&MS);
            SetCodeRange(FDataBlPtr, DP, DS);
            Fix0 = GetStartFixup(Ofs);
            SetStartFixupInfo(Fix0);
        }
        ShowGlobalTypeValue(hDT, DP, DS, true, -1, S);
        if (DP) RestoreFixupMemState(&MS);
    }
}
//------------------------------------------------------------------------------
void __fastcall TStrConstDecl::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    Action(this, hDT, IP);
}
//------------------------------------------------------------------------------
__fastcall TLabelDecl::TLabelDecl():TNameDecl(true)
{
    Ofs = ReadUIndex();
    if (FVer >= verD8 && FVer < verK1) ReadUIndex();
    if (FVer >= verD2009 && FVer < verK1) ReadUIndex();
}
//------------------------------------------------------------------------------
void __fastcall TLabelDecl::Show(String& OutS)
{
    TNameDecl::Show(OutS);
}
//------------------------------------------------------------------------------
BYTE __fastcall TLabelDecl::GetSecKind()
{
    return skLabel;
}
//------------------------------------------------------------------------------
bool __fastcall TLabelDecl::IsVisible(BYTE LK)
{
    return (LK != dlMain);
}
//------------------------------------------------------------------------------
__fastcall TExportDecl::TExportDecl():TNameDecl(true)
{
    hSym = ReadUIndex();
    Index = ReadUIndex();
}
//------------------------------------------------------------------------------
void __fastcall TExportDecl::Show(String& OutS)
{
    TDCURec *D;
    PName N, Name;

    D = GetAddrDef(hSym);
    N = NULL;
    if (!D)
    {
        OutLog1("?");
    }
    else
    {
        D->ShowName(OutS);
        N = D->GetName();
    }
    String sN = PName2String(N);
    Name = GetName();
    String sName = PName2String(Name);
    if (N && Name && sN != sName)
    {
        OutLog1(" name ");
        ShowName(OutS);
    }
    if (Index)
    {
        OutLog2(" index %lX", Index);
    }
}
//------------------------------------------------------------------------------
BYTE __fastcall TExportDecl::GetSecKind()
{
    return skExport;
}
//------------------------------------------------------------------------------
__fastcall TLocalDecl::TLocalDecl(BYTE LK):TNameDecl(true)
{
    bool    M, M2;

    BYTE tg = GetTag();
    M = (tg == arMethod || tg == arConstr || tg == arDestr);
    M2 = ((FVer == verD2) & M);
    LocFlags = ReadUIndex();
    if (FVer >= verD8 && FVer < verK1)
    {
        LocFlagsX = ReadUIndex();
        LocFlagsX = ((LocFlagsX & ~lfClassV8up) << 1) | ((LocFlagsX & lfClassV8up) >> 4); //To make the constants compatible with the previous versions
    }
    else
        LocFlagsX = LocFlags; //To simplify the rest of the code
    LocFlagsX &= ~lfauxPropField;
    if (FVer >= verD2009 && FVer < verK1) ReadUIndex();
    if (!M2)
        hDT = ReadUIndex();
    else if (M)
        Ndx = ReadUIndex();
    else
        Ndx = ReadIndex();
    if (LK == dlInterface || LK == dlDispInterface)
        NdxB = ReadUIndex();
    else
        NdxB = -1;
    if (!M2)
    {
        if (M)
            Ndx = ReadUIndex();
        else
            Ndx = ReadIndex();
    }
    else
        hDT = ReadIndex();//ReadUIndex()
    if (LK != dlClass && LK != dlInterface && LK != dlDispInterface && LK != dlFields)
    {
        tg = GetTag();
        if (tg == arFld || tg == arMethod || tg == arConstr || tg == arDestr) return;
    }
    if (GetTag() == arAbsLocVar)
        RefAddrDef(Ndx); //forward references could happen e.g. by referencing Self in embedded proc
}
//------------------------------------------------------------------------------
String RegName[7] = {"EAX", "EDX", "ECX", "EBX", "ESI", "EDI", "EBP"};
void __fastcall TLocalDecl::Show(String& OutS)
{
    BYTE Tg = GetTag();
    PName RefName;
    String MS, S;

    OutS = "";
    PLOCALDECLINFO info = new LOCALDECLINFO;
    info->Scope = ActiveScope;
    info->Tag = Tg;
    info->LocFlags = LocFlags;
    info->Ndx = Ndx;
    info->NdxB = NdxB;
    info->Case = CaseN;
    
    switch (Tg)
    {
    case arVal:
        MS = "val ";
        break;
    case arVar:
        MS = "var ";
        break;
    case drVar:
        MS = "local ";
        break;
    case arResult:
        MS = "result ";
        break;
    case arAbsLocVar:
        MS = "local absolute ";
        break;
    case arFld:
        MS = "field ";
        break;
    default:
        MS = "";
        break;
    }
    if (MS != "")
    {
        OutS += MS;
        OutLog2("%s", MS.c_str());
    }
    TNameDecl::Show(S);
    info->Name = S;
    OutS += S + ":";
    OutLog1(":");
    S = ShowTypeDef(hDT, NULL);
    info->TypeDef = S;
    info->AbsName = "";
    OutS += S;
    if ((LocFlags & 8) != 0 && Tg != arFld)
    {
        if (Ndx >= 0 && Ndx <= 6)
        {
            OutLog2("{%s}", RegName[Ndx].c_str());
        }
        else
        {
            OutLog1("{?}");
        }
    }
    else
        OutLog2("{Ofs:%d}", (int)Ndx);
    if (Tg == arAbsLocVar)
    {
        S = GetAddrStr((int)Ndx);
        OutS += " absolute " + S;
        OutLog2(" absolute %s", S.c_str());
        info->AbsName = S;
    }
    if (FieldsList)
        FieldsList->Add((void*)info);
    else if (ArgsList)
        ArgsList->Add((void*)info);
    else if (LocalsList)
        LocalsList->Add((void*)info);
    else
        delete info;
}
//------------------------------------------------------------------------------
void __fastcall TLocalDecl::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    Action(this, hDT, IP);
}
//------------------------------------------------------------------------------
BYTE __fastcall TLocalDecl::GetLocFlagsSecKind()
{
    switch (LocFlags & lfScope)
    {
    case lfPrivate:
        return skPrivate;
    case lfProtected:
        return skProtected;
    case lfPublic:
        return skPublic;
    case lfPublished:
        return skPublished;
    }
    return skNone;
}
//------------------------------------------------------------------------------
BYTE __fastcall TLocalDecl::GetSecKind()
{
    BYTE tg = GetTag();
    if (tg == arFld || tg == arMethod || tg == arConstr || tg == arDestr || tg == arProperty || tg == arClassVar)
        return GetLocFlagsSecKind();
    tg = GetTag();
    if (tg == arResult || tg == drVar || tg == arAbsLocVar)
        return skVar;
    return skNone;
}
//------------------------------------------------------------------------------
__fastcall TMethodDecl::TMethodDecl(BYTE LK):TLocalDecl(LK)
{
    PName name;
    InIntrf = (LK == dlInterface || LK == dlDispInterface);
    if (!InIntrf)
    {
        if (IsMSIL && Ndx)
            ReadByteIfEQ(1);//I was unable to find something less perverse to skip this byte
        if (FVer >= verD2009 && FVer < verK1 && GetTag() != arMethod)
            ReadByte();

        name = TNameDecl::GetName();
        if (FVer >= verD7 && FVer < verK1 || name->Len == 0)
            hImport = ReadUIndex(); //then hDT seems to be valid index in the parent class unit

        if (FVer >= verD2009 && FVer < verK1 && GetTag() == arMethod)
        {
            while (ReadByteFrom(FVer >= verD2010) >= 0);
        }
    }
}
//------------------------------------------------------------------------------
void __fastcall ShowFlags()
{
}
//------------------------------------------------------------------------------
void __fastcall TMethodDecl::Show(String& OutS)
{
    String MS, PS, S;
    TDCURec *D;
    BYTE Tg = GetTag(), MK;    //TMethodKind

    PMETHODDECLINFO MethodDeclInfo = new METHODDECLINFO;
    MethodDeclInfo->Scope = ActiveScope;
    OutS = "";
    if ((LocFlags & lfClass) != 0)
    {
        PS = "class ";
        OutS += "class ";
        OutLog1("class ");
    }

    if (Ndx || !IsMSIL)
    {
        D = GetAddrDef(Ndx);
        if (D && !D->InheritsFrom(__classid(TProcDecl)))
            D = NULL;
        if (D)
        {
            switch (Tg)
            {
            case arMethod:
                MK = mkMethod;
                break;
            case arConstr:
                MK = mkConstructor;
                break;
            case arDestr:
                MK = mkDestructor;
                break;
            }
            ((TProcDecl*)D)->MethodKind = MK;
        }
    }
    switch (Tg)
    {
    case arMethod:
        if (!D)
        {
            MS = "method ";
            MethodDeclInfo->MethodKind = 'M';
        }
        else if (((TProcDecl*)D)->IsProc())
        {
            MS = "procedure ";
            MethodDeclInfo->MethodKind = 'P';
        }
        else
        {
            MS = "function ";
            MethodDeclInfo->MethodKind = 'F';
        }
        break;
    case arConstr:
        MS = "constructor ";
        MethodDeclInfo->MethodKind = 'C';
        break;
    case arDestr:
        MS = "destructor ";
        MethodDeclInfo->MethodKind = 'D';
        break;
    }
    MethodDeclInfo->Prototype = "";
    if (!InIntrf && (Ndx || !IsMSIL))
    {
        if (MS != "")
        {
            PS += MS; OutS += MS;
            OutLog2("%s", MS.c_str());
        }
        ShowName(S);
        PS += S; OutS += S;

        if (!D)
        {
            OutS += ":"; PS += ":";
            OutLog1(":");
        }

        ShowFlags();
        if (D)
            ((TProcDecl*)D)->ShowArgs(S, 0);
        else
        {
            S = GetAddrStr(Ndx);
            OutLog2("%s", S.c_str());
        }
        PS += S; OutS += S;

        if ((LocFlags & lfOverride) != 0)
        {
            PS += ";override"; OutS += ";override";
            OutLog1(";override{");
            if ((LocFlags & lfVirtual) != 0)
                OutLog1(";virtual");
            if ((LocFlags & lfDynamic) != 0)
                OutLog1(";dynamic");
            OutLog1("}");
        }
        else
        {
            if ((LocFlags & lfVirtual) != 0)
            {
                PS += ";virtual"; OutS += ";virtual";
                OutLog1(";virtual");
            }
            if ((LocFlags & lfDynamic) != 0)
            {
                PS += ";dynamic"; OutS += ";dynamic";
                OutLog1(";dynamic");
            }
        }
        MethodDeclInfo->Prototype = PS;
    }
    else
    {
        if (MS != "")
        {
            PS += MS; OutS += MS;
            OutLog2("%s", MS.c_str());
        }
        if (!Ndx && IsMSIL)
            D = GetTypeDef(hImport);
        else
            D = GetTypeDef(Ndx);
        if (D && D->InheritsFrom(__classid(TProcTypeDef)))
        {
            S = ((TProcTypeDef*)D)->ProcStr();
            if (S[1] == 'p') MethodDeclInfo->MethodKind = 'P';
            else if (S[1] == 'f') MethodDeclInfo->MethodKind = 'F';
            PS += S + " "; OutS += S + " ";
            OutLog2("%s ", S.c_str());
            ShowName(S);
            PS += S; OutS += S;
            ((TProcTypeDef*)D)->ShowDecl("()", S);
            PS += S; OutS += S;
            ShowFlags();
            MethodDeclInfo->Prototype = PS;
        }
        else
        {
            ShowName(S);
            PS += S + ":"; OutS += S + ":";
            OutLog1(": ");
            ShowFlags();
            S = ShowTypeDef(Ndx, GetName());
            PS += S; OutS += S;
            MethodDeclInfo->Prototype = PS;
        }
    }
    if (MethodsList) MethodsList->Add((void*)MethodDeclInfo);
}
//------------------------------------------------------------------------------
__fastcall TClassVarDecl::TClassVarDecl(BYTE LK) : TLocalDecl(LK)
{
}
//------------------------------------------------------------------------------
void __fastcall TClassVarDecl::Show(String& OutS)
{
    OutLog1("class var ");
    TLocalDecl::Show(OutS);
}
//------------------------------------------------------------------------------
BYTE __fastcall TClassVarDecl::GetSecKind()
{
    return GetLocFlagsSecKind();
}
//------------------------------------------------------------------------------
__fastcall TPropDecl::TPropDecl() : TNameDecl(true)
{
    int     X, X1, X2, X3, X4, Flags1;

    LocFlags = ReadIndex();
    if (FVer >= verD8 && FVer < verK1)
    {
        LocFlagsX = ReadUIndex();
        LocFlagsX = ((LocFlagsX & ~lfClassV8up) << 1) | ((LocFlagsX & lfClassV8up) >> 4); //To make the constants compatible with the previous versions
    }
    else
        LocFlagsX = LocFlags; //To simplify the rest of the code
    if (FVer >= verD2009 && FVer < verK1) X4 = ReadUIndex();
    
    hDT = ReadUIndex();
    Ndx = ReadIndex();
    hIndex = ReadIndex();
    hRead = ReadUIndex();
    hWrite = ReadUIndex();
    hStored = ReadUIndex();
    //forward references could happen by mentioning parent methods or fields
    //when defining child properties and when child definition goes before parent in DCU
    //due to usage of TChild = class; before TParent definition
    if (hRead) RefAddrDef(hRead);
    if (hWrite) RefAddrDef(hWrite);
    if (hStored) RefAddrDef(hStored);
    if (FVer >= verD8 && FVer < verK1)
    {
        X = ReadUIndex();
        X1 = ReadUIndex();
        if (IsMSIL)
        {
            X2 = ReadUIndex();
            X3 = ReadUIndex();
        }
    }
    hDeft = ReadIndex();
}
//------------------------------------------------------------------------------
String __fastcall TPropDecl::PutOp(String Name, int hOp)
{
    String V;

    if (!hOp) return "";
    V = GetAddrStr(hOp);
    OutLog3(" %s %s", Name.c_str(), V.c_str());
    return V;
}
//------------------------------------------------------------------------------
void __fastcall TPropDecl::Show(String& OutS)
{
    TBaseDef *D;
    int hDT0;
    String S;

    OutS = "";
    PPROPERTYINFO pInfo = new PROPERTYINFO;
    pInfo->Scope = ActiveScope;
    pInfo->Index = hIndex;
    pInfo->DispId = 0;
    pInfo->TypeDef = "";
    OutLog1("property ");
    TNameDecl::Show(S);
    pInfo->Name = S;
    if (hDT)
    {
        D = GetTypeDef(hDT);
        if (D && D->InheritsFrom(__classid(TProcTypeDef)) && !D->FName)
        {
            ((TProcTypeDef*)D)->ShowDecl("[]", OutS);
        }
        else
        {
            OutLog1(":");
            pInfo->TypeDef = ShowTypeDef(hDT, NULL);
        }
    }
    if (hIndex != (int)0x80000000) OutLog2("index %lX", hIndex);
    pInfo->ReadName = PutOp("read", hRead);
    pInfo->WriteName = PutOp("write", hWrite);
    pInfo->StoredName = PutOp("stored", hStored);

    if (PropertiesList)
        PropertiesList->Add((void*)pInfo);
}
//------------------------------------------------------------------------------
void __fastcall TPropDecl::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    Action(this, hDT, IP);
}
//------------------------------------------------------------------------------
BYTE __fastcall TPropDecl::GetSecKind()
{
    switch (LocFlags & lfScope)
    {
    case lfPrivate:
        return skPrivate;
    case lfProtected:
        return skProtected;
    case lfPublic:
        return skPublic;
    case lfPublished:
        return skPublished;
    }
    return skNone;
}
//------------------------------------------------------------------------------
__fastcall TDispPropDecl::TDispPropDecl(BYTE LK) : TLocalDecl(LK)
{
}
//------------------------------------------------------------------------------
void __fastcall TDispPropDecl::Show(String& OutS)
{
    String S;

    PPROPERTYINFO pInfo = new PROPERTYINFO;
    pInfo->Scope = ActiveScope;
    pInfo->Index = NdxB;
    pInfo->DispId = Ndx;
    pInfo->ReadName = "";
    pInfo->WriteName = "";
    pInfo->StoredName = "";

    OutLog1("property ");
    ShowName(S);
    pInfo->Name = S;
    OutLog1(": ");
    pInfo->TypeDef = ShowTypeDef(hDT, NULL);

    if (NdxB != -1)
    {
        switch (NdxB & 6)
        {
        case 2:
            OutLog1(" readonly");
            break;
        case 4:
            OutLog1(" writeonly");
            break;
        }
    }
    OutLog2(" dispid %lX", (int)Ndx);

    if (PropertiesList)
        PropertiesList->Add((void*)pInfo);

    OutS = "";
}
//------------------------------------------------------------------------------
__fastcall TConstDeclBase::TConstDeclBase() : TNameFDecl(false)
{
}
//------------------------------------------------------------------------------
void __fastcall TConstDeclBase::ReadConstVal()
{
    ValSz = ReadUIndex();
    if (!ValSz)
    {
        ValPtr = 0;
        Val = ReadIndex();
        ValSz = NDXHi;
    }
    else
    {
        ValPtr = CurPos;
        SkipBlock(ValSz);
        Val = 0;
    }
}
//------------------------------------------------------------------------------
void __fastcall TConstDeclBase::ShowValue(String& OutS)
{
    BYTE *DP;
    DWORD DS;
    TInt64Rec V;
    bool MemVal;
    String S, SV;

    OutS = "";
    if (!ValPtr)
    {
        V.Hi = ValSz;
        V.Lo = Val;
        DP = (BYTE*)&V;
        DS = 8;
    }
    else
    {
        DP = ValPtr;
        DS = ValSz;
    }
    MemVal = (ValPtr != NULL);
    if (ShowGlobalTypeValue(hDT, DP, DS, MemVal, Kind, S) < 0 && !MemVal)
    {
        S = ShowTypeName(hDT);
        NDXHi = V.Hi;
        SV = NDXToStr(V.Lo);
        OutLog2("%s", SV.c_str());
        S += SV;
    }
    OutS += S;
}
//------------------------------------------------------------------------------
void __fastcall TConstDeclBase::Show(String& OutS)
{
    PName RefName;
    bool TypeNamed;
    String S;

    PCONSTINFO constInfo = new CONSTINFO;
    constInfo->ModuleID = ModuleID;
    constInfo->Type = CI_CONSTDECL;
    constInfo->TypeDef = "";
    constInfo->RTTIOfs = 0xFFFFFFFF;
    constInfo->RTTISz = 0;
    constInfo->Fixups = new TList;

    OutS = "";
    TNameFDecl::Show(S);
    constInfo->Name = S;
    OutS += S + "=";
    OutLog1("=");

    pDumpOffset = &constInfo->RTTIOfs;
    pDumpSize = &constInfo->RTTISz;
    FixupsList = constInfo->Fixups;
    ShowValue(S);
    pDumpOffset = 0;
    pDumpSize = 0;
    FixupsList = 0;

    constInfo->Value = S;
    OutS += S;
    if (ConstList)
        ConstList->Add((void*)constInfo);
    else
    {
        delete constInfo->Fixups;
        delete constInfo;
    }
}
//------------------------------------------------------------------------------
void __fastcall TConstDeclBase::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    Action(this, hDT, IP);
}
//------------------------------------------------------------------------------
BYTE __fastcall TConstDeclBase::GetSecKind()
{
    return skConst;
}
//------------------------------------------------------------------------------
__fastcall TConstDecl::TConstDecl() : TConstDeclBase()
{
    hDT = ReadUIndex();
    if (FVer > verD4)
    {
        Kind = ReadUIndex();
        if (Kind < 0 || Kind > 5 || (Kind == 5 && !(FVer >= verD2009 && FVer < verK1)))
            printf("Unknown const kind: #%d\n", Kind);
    }
    ReadConstVal();
}
//------------------------------------------------------------------------------
bool __fastcall TConstDecl::IsVisible(BYTE LK)
{
    PName       NP;

    if (!Inf && (FVer <= verD4 || Kind == 1) && ValPtr && ValSz > 8 && (int)(*ValPtr) == -1)
    {
      NP = GetName();
      if (NP && NP->Name[0] == '.') return false; //The resourcestring value looks like this - it should be ignored
    }
    return TNameFDecl::IsVisible(LK);
}
//------------------------------------------------------------------------------
__fastcall TResStrDef::TResStrDef() : TVarCDecl(false)
{
    OfsR = Ofs;
    Ofs = -1;
}
//------------------------------------------------------------------------------
void __fastcall TResStrDef::Show(String& OutS)
{
//The reference to HInstance will be shown
    BYTE *DP;
    DWORD DS;
    int Fix0;
    TFixupMemState MS;
    String S;

    PRESSTRINFO rsInfo = new RESSTRINFO;
    rsInfo->ModuleID = ModuleID;
    rsInfo->DumpOfs = 0xFFFFFFFF;
    rsInfo->DumpSz = 0;

    TNameFDecl::Show(S);
    rsInfo->Name = S;
    OutLog1(":");
    rsInfo->TypeDef = ShowTypeDef(hDT, NULL);
    rsInfo->Context = "";

    ShowGlobalConstValue(hDecl + 1, OutS);
    ResStrList->Add((void*)rsInfo);
}
//------------------------------------------------------------------------------
BYTE __fastcall TResStrDef::GetSecKind()
{
    return skResStr;
}
//------------------------------------------------------------------------------
__fastcall TSetDeftInfo::TSetDeftInfo():TNameDecl()
{
    Def = (PNameDef)DefStart;
    hDecl = -1;
    hConst = ReadUIndex();
    hArg = ReadUIndex();
}
//------------------------------------------------------------------------------
void __fastcall TSetDeftInfo::Show(String& OutS)
{
    OutLog2("Let %s := ", GetAddrStr(hArg).c_str());
    ShowGlobalConstValue(hConst, OutS);
    OutLog1("\n");
}
//------------------------------------------------------------------------------
__fastcall TCopyDecl::TCopyDecl():TNameDecl(false)
{
    TDCURec* SrcDef;

    hBase = ReadUIndex(); //index of the address to copy from
    SrcDef = GetAddrDef(hBase);
    if (!SrcDef)
        printf("Error: CopyDecl index #%lX not found\n", hBase);
    if (!SrcDef->InheritsFrom(__classid(TNameDecl)))
        printf("Error: CopyDecl index #%lX(%s) is not a TNameDecl\n", hBase);
    Base = (TNameDecl*)SrcDef;
    Def = Base->Def;
}
//------------------------------------------------------------------------------
void __fastcall TCopyDecl::Show(String& OutS)
{
    Base->Show(OutS);
}
//------------------------------------------------------------------------------
BYTE __fastcall TCopyDecl::GetSecKind()
{
    return Base->GetSecKind();
}
//------------------------------------------------------------------------------
extern  BYTE    *FMemPtr;
__fastcall TProcDecl::TProcDecl(TNameDecl* AnEmbedded, bool NoInf) : TNameFDecl(NoInf)
{
    int         X;
    TNameDecl   **ArgP;
    TNameDecl   *Loc;
    
    CodeOfs = -1;
    Embedded = AnEmbedded;
    bool NoName = IsUnnamed();
    MethodKind = mkProc;
    Locals = NULL;
    B0 = ReadUIndex();
    Sz = ReadUIndex();
    if (FVer >= verDXE1 && FVer < verK1) X = ReadUIndex();
    if (!NoName)
    {
        if (FVer > verD2) VProc = ReadUIndex();
        hDTRes = ReadUIndex();
        if (FVer >= verDXE1 && FVer < verK1 && VProc == 0x4F && ((F1 & 0x40) != 0)) return;
        if (FVer > verD7 && FVer < verK1)
            hClass = ReadUIndex();
        Tag = ReadTag();
        CallKind = ReadCallKind();

        if (FVer > verD2009 && FVer < verK1)
        {
            //Read template parameters
            if (Tag == drA5Info) Tag = ReadTag(); //always precedes drA6Info
            if (Tag == drA6Info)
            {
                FTemplateArgs = new TA6Def;
                Tag = ReadTag();
            }
        }

        ReadDeclList(dlArgs, &Args);
        if (Tag != drStop1)
            printf("Error: Stop Tag\n");
        ArgP = &Args;
        while (*ArgP)
        {
            Loc = *ArgP;
            BYTE tg = Loc->GetTag();
            if (tg != arVal && tg != arVar) break;
            ArgP = &(TNameDecl*)Loc->Next;
        }
        Locals = *ArgP;
        *ArgP = NULL;
    }
}
//------------------------------------------------------------------------------
__fastcall TProcDecl::~TProcDecl()
{
    FreeDCURecList((TDCURec*)Locals);
    FreeDCURecList((TDCURec*)Args);
    FreeDCURecList((TDCURec*)Embedded);
}
//------------------------------------------------------------------------------
//In Kylix are used the names of the kind '.<X>.'
//In Delphi 6 were noticed only names '..'
//In Delphi 9 were noticed names of the kind '.<X>'
bool __fastcall TProcDecl::IsUnnamed()
{
    bool Result = (Def->Name.Len == 0) || (Def->Name.Len == 1 && Def->Name.Name[0] == '.')
        || (FVer >= verD6) && (FVer < verK1) && (Def->Name.Len == 2 && Def->Name.Name[0] == '.' && Def->Name.Name[1] == '.')
        || ((FVer >= verK1) || (FVer >= verD8)) && (Def->Name.Name[0] == '.');
    return Result;
}
//------------------------------------------------------------------------------
DWORD __fastcall TProcDecl::SetMem(DWORD MOfs, DWORD MSz)
{
    CodeOfs = MOfs;
    return MSz - Sz;
}
//------------------------------------------------------------------------------
BYTE __fastcall TProcDecl::GetSecKind()
{
    return skProc;
}
//------------------------------------------------------------------------------
String CallKindName[5] = {"register", "cdecl", "pascal", "stdcall", "safecall"};
void __fastcall TProcDecl::ShowArgs(String& OutS, PPROCDECLINFO pInfo)
{
    bool NoName;
    DWORD Ofs0;
    TNameDecl *ArgL;
    String S = "";

    OutS = "";
    if (FTemplateArgs)
    {
        OutS += "<";
        FTemplateArgs->Show(S);
        OutS += S;
        OutS += ">";
    }
    NoName = IsUnnamed();
    ArgL = Args;
    if (ArgL)
    {
        OutS += "(";
        OutLog1("(");
    }
    ShowDeclList(dlArgs, ArgL, S);
    OutS += S;
    if (ArgL)
    {
        OutS += ")";
        OutLog1(")");
    }
    if (!IsProc())
    {
        OutS += ":";
        OutLog1(":");
        S = ShowTypeDef(hDTRes, NULL);
        if (pInfo) pInfo->TypeDef = S;
        OutS += S;
    }
    if (CallKind != pcRegister)
    {
        OutS += ";";
        OutLog1(";");
        S = CallKindName[CallKind];
        OutS += S;
        OutLog2("%s", S.c_str());
    }
    if (FVer > verD3)
    {
        if (FVer < verD2005)
        {
            if ((VProc & 0x1000) != 0)
            {
                OutS += ";overload";
                OutLog1(";overload");
            }
        }
        else
        {
            if ((VProc & 0x800) != 0)
            {
                OutS += ";overload";
                OutLog1(";overload");
            }
            if ((VProc & 0x2000000) != 0)
            {
                OutS += ";inline";
                OutLog1(";inline");
            }
        }
    }
}
//------------------------------------------------------------------------------
bool __fastcall TProcDecl::IsProc()
{
    return TypeIsVoid(hDTRes);
}
//------------------------------------------------------------------------------
void __fastcall TProcDecl::ShowDef(bool All, String& OutS)
{
    String S = "";

    PPROCDECLINFO pInfo = new PROCDECLINFO;
    pInfo->ModuleID = ModuleID;
    pInfo->Embedded = false;
    pInfo->CallKind = CallKind;
    pInfo->DumpOfs = 0xFFFFFFFF;
    pInfo->DumpSz = 0;
    pInfo->VProc = VProc;

    if (IsProc())
    {
        switch (MethodKind)
        {
        case mkConstructor:
            pInfo->MethodKind = 'C';
            OutLog1("constructor ");
            break;
        case mkDestructor:
            pInfo->MethodKind = 'D';
            OutLog1("destructor ");
            break;
        default:
            pInfo->MethodKind = 'P';
            OutLog1("procedure ");
            break;
        }
    }
    else
    {
        pInfo->MethodKind = 'F';
        OutLog1("function ");
    }
    TNameFDecl::Show(S);
    pInfo->Name = S;

    if (Def->Name.Len == 0) OutLog1("?");

    pInfo->Args = new TList;
    ArgsList = pInfo->Args;
    ShowArgs(S, pInfo);
    ArgsList = 0;
    if (All)
    {
        pInfo->Locals = new TList;
        pInfo->Fixups = new TList;

        OutLog1(";\n");
        if (Locals)
        {
            LocalsList = pInfo->Locals;
            ShowDeclList(dlEmbedded, Locals, S);
            LocalsList = 0;
        }
        if (Embedded)
        {
            pInfo->Embedded = true;
            ShowDeclList(dlEmbedded, Embedded, S);
        }
        OutLog1("begin ");
        //GetRegVarInfo = GetRegDebugInfo;

        pDumpOffset = &pInfo->DumpOfs;
        pDumpSize = &pInfo->DumpSz;
        FixupsList = pInfo->Fixups;

        if (!IsUnnamed())
        {
            pInfo->DumpType = 'C';
            OutLog1("code\n");
            ShowCodeBl(AddrBase, CodeOfs, Sz);
        }
        else
        {
            pInfo->DumpType = 'D';
            OutLog1("data\n");
            ShowDataBl(AddrBase, CodeOfs, Sz);
        }
        pDumpOffset = 0;
        pDumpSize = 0;
        FixupsList = 0;

        //GetRegVarInfo = NULL;
        ProcList->Add((void*)pInfo);
        OutLog1("end");
    }
    else
    {
        delete pInfo->Args;
        delete pInfo;
    }
    OutLog1("\n");
}
//------------------------------------------------------------------------------
void __fastcall TProcDecl::Show(String& OutS)
{
    ShowDef(true, OutS);
}
//------------------------------------------------------------------------------
void __fastcall TProcDecl::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    EnumUsedTypeList(Args, Action, IP);
    if (!IsProc()) Action(this, hDTRes, IP);
}
//------------------------------------------------------------------------------
bool __fastcall TProcDecl::IsVisible(BYTE LK)
{
    if (LK == dlMain) return (((F & 0x40) != 0) && (MethodKind == mkProc) && (hClass == 0));
    return true;
}
//------------------------------------------------------------------------------
__fastcall TSysProcDecl::TSysProcDecl() : TNameDecl(true)
{
    F = ReadUIndex();
    Ndx = ReadIndex();
}
//------------------------------------------------------------------------------
void __fastcall TSysProcDecl::Show(String& OutS)
{
    OutLog1("sysproc ");
    TNameDecl::Show(OutS);
}
//------------------------------------------------------------------------------
BYTE __fastcall TSysProcDecl::GetSecKind()
{
    return skProc;
}
//------------------------------------------------------------------------------
__fastcall TSysProc8Decl::TSysProc8Decl() : TProcDecl(NULL, true)
{
}
//------------------------------------------------------------------------------
__fastcall TUnitAddInfo::TUnitAddInfo() : TNameFDecl(false)
{
    B = ReadUIndex();
    Tag = ReadTag();
    ReadDeclList(dlUnitAddInfo, &Sub);
}
//------------------------------------------------------------------------------
__fastcall TUnitAddInfo::~TUnitAddInfo()
{
    FreeDCURecList((TDCURec*)Sub);    
}
//------------------------------------------------------------------------------
bool __fastcall TUnitAddInfo::IsVisible(BYTE LK)
{
    return  false;
}
//------------------------------------------------------------------------------
__fastcall TSpecVar::TSpecVar() : TVarDecl()
{
}
//------------------------------------------------------------------------------
void __fastcall TSpecVar::Show(String& OutS)
{
    String  S;

    OutLog1("spec var ");

    PVARINFO vInfo = new VARINFO;
    vInfo->ModuleID = ModuleID;
    vInfo->Type = VI_SPECVAR;
    vInfo->DumpOfs = 0xFFFFFFFF;
    vInfo->DumpSz = 0;
    vInfo->AbsName = "";

    TNameFDecl::Show(S);
    vInfo->Name = S;
    OutS = S + ":";
    OutLog1(":");
    pDumpOffset = &vInfo->DumpOfs;
    pDumpSize = &vInfo->DumpSz;
    S = ShowTypeDef(hDT, NULL);
    pDumpOffset = 0;
    pDumpSize = 0;

    vInfo->TypeDef = S;
    OutS += S;
    VarList->Add((void*)vInfo);
}
//------------------------------------------------------------------------------
__fastcall TTypeDef::TTypeDef() : TBaseDef(NULL, (PNameDef)DefStart, -1)
{
    RTTISz = ReadUIndex();
    Sz = ReadIndex();
    hAddrDef = ReadUIndex();
    if (IsMSIL)
    {
        ReadUIndex();
        ReadUIndex();
    }
    else if (FVer >= verD2005 && FVer < verK1)
        X = ReadUIndex();
    AddTypeDef(this);
    RTTIOfs = -1;
}
//------------------------------------------------------------------------------
__fastcall TTypeDef::~TTypeDef()
{
    //???
}
//------------------------------------------------------------------------------
void __fastcall TTypeDef::ShowBase()
{
    if (RTTISz > 0) ShowDataBl(0, RTTIOfs, RTTISz);
}
//------------------------------------------------------------------------------
int __fastcall TTypeDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    OutS = "";
    if (Sz > DS) return -1;
    ShowDump(DP, NULL, 0, 0, Sz, 0, 0, 0, 0, NULL);
    return Sz;
}
//------------------------------------------------------------------------------
void __fastcall TTypeDef::Show(String& OutS)
{
    ShowBase();
    OutS = "";
}
//------------------------------------------------------------------------------
DWORD __fastcall TTypeDef::SetMem(DWORD MOfs, DWORD MSz)
{
    RTTIOfs = MOfs;
    return 0;
}
//------------------------------------------------------------------------------
String __fastcall TTypeDef::GetOfsQualifier(int Ofs)
{
    if (Ofs == 0) return "";
    if (Ofs < Sz) return Format(".byte[%d]", ARRAYOFCONST((Ofs)));
    return Format(".?%d", ARRAYOFCONST((Ofs))); //Error
}
//------------------------------------------------------------------------------
String __fastcall TTypeDef::GetRefOfsQualifier(int Ofs)
{
    if (Ofs == 0) return "^";
    return Format(".?%d", ARRAYOFCONST((Ofs))); //Error
}
//------------------------------------------------------------------------------
__fastcall TRangeBaseDef::TRangeBaseDef() : TTypeDef()
{
}
//------------------------------------------------------------------------------
void __fastcall TRangeBaseDef::GetRange(PInt64Rec Lo, PInt64Rec Hi)
{
    BYTE *Tmp = CurPos;
    CurPos = LH;
    ReadIndex64(Lo);
    ReadIndex64(Hi);
    CurPos = Tmp;
}
//------------------------------------------------------------------------------
String __fastcall WCharStr(wchar_t WCh)
{
    wchar_t WStr[2];
    String S;
    char Ch, buf[256];

    if ((WORD)WCh < 0x100)
        Ch = WCh;
    else
    {
        WStr[0] = WCh;
        WStr[1] = 0;
        S = WideCharToString(WStr);
        if (S.Length() > 0)
            Ch = S[1];
        else
            Ch = '.';
    }
    if (Ch < ' ')
        sprintf(buf, "#%d", (WORD)WCh);
    else
        sprintf(buf, "#$%lX", (WORD)WCh);
    return String(buf);
}
//------------------------------------------------------------------------------
String __fastcall BoolStr(char* DP, DWORD DS)
{
    char *CP;

    CP = DP + DS - 1;
    while (CP > DP && *CP == 0) CP--;
    if (CP = DP)
    {
        if (*CP == 0) return "false";
        if (*CP == 1) return "true";
    }
    return "true";
}
//------------------------------------------------------------------------------
typedef struct
{
    char    Ch0;
    char    Ch1;
} TByteChars;

char Digit[16] = {
'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
};

WORD __fastcall ByteChars(BYTE B)
{
    TByteChars Ch;
    WORD* Result = (WORD*)&Ch;

    Ch.Ch0 = Digit[B >> 4];
    Ch.Ch1 = Digit[B & 0xF];
    return *Result;
}
//------------------------------------------------------------------------------
String __fastcall IntLStr(BYTE* DP, DWORD Sz, bool Neg)
{
    int i;
    BYTE *BP;
    char *P;
    int V;
    bool Ok;
    char buf[256];

    if (Neg)
    {
        Ok = true;
        switch (Sz)
        {
        case 1:
            V = *((char*)DP);
            break;
        case 2:
            V = *((short*)DP);
            break;
        case 4:
            V = *((int*)DP);
            break;
        default:
            Ok = false;
            if (Sz == 8)
            {
              V = *((int*)DP);
              DP += 4;
              NDXHi = *((int*)DP);
              return NDXToStr(V);
            }
            break;
        }
        if (Ok)
        {
            if (V >= 0)
                sprintf(buf, "$%lX", V);
            else
                sprintf(buf, "-$%lX", -V);
            return String(buf);
        }
    }
    BP = DP + Sz - 1;
    String Result;
    Result.SetLength(Sz*2 + 1);
    P = Result.c_str();
    *P = '$';
    P++;
    for (i = 1; i <= Sz; i++)
    {
        *((WORD*)P) = ByteChars(*BP);
        P += 2;
        BP--;
    }
    return Result;
}
//------------------------------------------------------------------------------
int __fastcall TRangeBaseDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    BYTE *Tmp;
    bool Neg;
    int Lo;
    BYTE Tag;

    OutS = "";
    if (Sz > DS) return -1;
    int Result = Sz;
    if (!Def)
        Tag = drRangeDef;
    else
        Tag = Def->Tag;
    switch (Tag)
    {
    case drChRangeDef:
        if (Sz == 1)
        {
            OutS = CharStr(*((char*)DP));
            OutLog2("%s", OutS.c_str());
            return 1;
        }
        break;
    case drWCharRangeDef:
        if (Sz == 2)
        {
            OutS = WCharStr(*((wchar_t*)DP));
            OutLog2("%s", OutS.c_str());
            return 2;
        }
        break;
    case drBoolRangeDef:
        OutS = BoolStr(DP, Sz);
        OutLog2("%s", OutS.c_str());
        return Sz;
    }
    Tmp = CurPos;
    CurPos = LH;
    Lo = ReadIndex();
    Neg = (NDXHi < 0);
    CurPos = Tmp;
    OutS = IntLStr(DP, Sz, Neg);
    OutLog2("%s", OutS.c_str());
    return Sz;
}
//------------------------------------------------------------------------------
void __fastcall TRangeBaseDef::Show(String& OutS)
{
    TInt64Rec Lo, Hi;
    TTypeDef *T;
    String Value, S;

    OutS = "";
    TTypeDef::Show(OutS);
    GetRange(&Lo, &Hi);
    T = GetGlobalTypeDef(hDTBase);

    if (!T || ShowTypeValue(T, (BYTE*)&Lo, 8, 0, S) < 0)
    {
        NDXHi = Lo.Hi;
        Value = NDXToStr(Lo.Lo);
        S = Value;
        OutLog2("%s", Value.c_str());
    }
    OutS += S;

    OutS += "..";
    OutLog1("..");

    if (!T || ShowTypeValue(T, (BYTE*)&Hi, 8, 0, S) < 0)
    {
        NDXHi = Hi.Hi;
        Value = NDXToStr(Hi.Lo);
        S = Value;
        OutLog2("%s", Value.c_str());
    }
    OutS += S;
}
//------------------------------------------------------------------------------
void __fastcall TRangeBaseDef::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    Action(this, hDTBase, IP);
}
//------------------------------------------------------------------------------
__fastcall TRangeDef::TRangeDef() : TRangeBaseDef()
{
    DWORD Lo, Hi;

    hDTBase = ReadUIndex();
    LH = CurPos;
    Lo = ReadIndex();
    Hi = ReadIndex();
    if (FVer >= verD8 && FVer < verK1)
        B = ReadUIndex();
    else
        B = ReadByte(); //It could be index too, but I'm not sure
}
//------------------------------------------------------------------------------
__fastcall TEnumDef::TEnumDef() : TRangeBaseDef()
{
    DWORD Lo, Hi;

    hDTBase = ReadUIndex();
    if (FVer >= verD2009 && FVer < verK1) ReadUIndex();
    Ndx = ReadIndex();
    LH = CurPos;
    Lo = ReadIndex();
    Hi = ReadIndex();
    if (FVer >= verD8 && FVer < verK1)
        B = ReadUIndex();
    else
        B = ReadByte(); //It could be index too, but I'm not sure
}
//------------------------------------------------------------------------------
__fastcall TEnumDef::~TEnumDef()
{
    if (NameTbl)
    {
        if (NameTbl->Count > 0)
            FreeDCURecList((TDCURec*)(NameTbl->Items[0]));
        delete NameTbl;
    }
}
//------------------------------------------------------------------------------
int __fastcall TEnumDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    DWORD V;
    String S;

    OutS = "";
    if (Sz > DS) return -1;
    if (!MemToUInt(DP, Sz, &V) || V < 0 || !NameTbl || V >= NameTbl->Count)
    {
        ShowName(S);
        OutS += S + "(";
        OutLog1("(");
        TRangeBaseDef::ShowValue(DP, DS, S);
        OutS += S + ")";
        OutLog1(")");
        return Sz;
    }
    ((TConstDecl*)(NameTbl->Items[V]))->ShowName(OutS);
    return Sz;
}
//------------------------------------------------------------------------------
void __fastcall TEnumDef::Show(String& OutS)
{
    TNameDecl *EnumConst;
    int i;
    String Name;

    if (!NameTbl)
    {
        TRangeBaseDef::Show(OutS);
        return;
    }
    ShowBase();
    OutS = "(";
    OutLog1("(");
    for (i = 0; i < NameTbl->Count; i++)
    {
        if (i > 0)
        {
            OutS += ",";
            OutLog1(",");
        }
        EnumConst = (TNameDecl*)NameTbl->Items[i];
        Name = PName2String(EnumConst->GetName());
        OutS += Name;
        OutLog2("%s", Name.c_str());
    }
    OutS += ")";
    OutLog1(");");
}
//------------------------------------------------------------------------------
__fastcall TFloatDef::TFloatDef() : TTypeDef()
{
    BYTE B = ReadByte();
    Kind = B;
}
//------------------------------------------------------------------------------
String __fastcall TFloatDef::GetKindName()
{
    switch (Kind)
    {
    case 0:
        return "fkReal48";
    case 1:
        return "fkSingle";
    case 2:
        return "fkDouble";
    case 3:
        return "fkExtended";
    case 4:
        return "fkComp";
    case 5:
        return "fkCurrency";
    }
}
//------------------------------------------------------------------------------
int __fastcall TFloatDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    long double E;
    PName N;
    bool Ok;

    OutS = "";
    if (Sz > DS) return -1;
    Ok = true;
    switch (DS)
    {
    case 4: //SizeOf(Single)
        E = *((float*)DP);
        break;
    case 8: //SizeOf(Double)
        N = GetName();
        if (!N)
            Ok = false;
        else
        {
            if (!CompareText(PName2String(N), "Double"))
                E = *((double*)DP);
            else if (!CompareText(PName2String(N), "Currency"))
                E = *((Currency*)DP);
            else if (!CompareText(PName2String(N), "Comp"))
                E = *((Comp*)DP);
            else
                Ok = false;
        }
        break;
    case 10:    //SizeOf(Extended)
        E = *((long double*)DP);
        break;
    default:
        Ok = false;
        break;
    }
    if (Ok)
    {
        OutS = Format("%g", ARRAYOFCONST((E)));
        OutLog2("%s", OutS.c_str());
        return Sz;
    }
    return TTypeDef::ShowValue(DP, Sz, OutS);
}
//------------------------------------------------------------------------------
void __fastcall TFloatDef::Show(String& OutS)
{
    String      S;

    OutS = "float(" + GetKindName() + ")";
    OutLog2("float(%s)", OutS.c_str());
    TTypeDef::Show(S);
    OutS += S;
}
//------------------------------------------------------------------------------
__fastcall TPtrDef::TPtrDef() : TTypeDef()
{
    hRefDT = ReadUIndex();
    if (FVer >= verD2009 && FVer < verK1) ReadUIndex();
}
//------------------------------------------------------------------------------
//Set FixUpEnd to the max(FixUpEnd,CodeFixups^.Ofs+4 if CodeFixups^.F is not fxStart or fxEnd
void __fastcall SetFixEnd()
{
    DWORD CurOfs;
    BYTE F;
    BYTE *EP;

    CurOfs = CodeFixups->OfsF;
    F = ((BYTE*)&CurOfs)[3];
    CurOfs = CurOfs & FixOfsMask;
    if (F < fxStart)
    {
        EP = CodeStart + CurOfs+4;
        if (EP > FixUpEnd) FixUpEnd = EP;
    }
}
//------------------------------------------------------------------------------
//Move CodeFixups to the next fixup with Offset>=Ofs
void __fastcall SkipFixups(DWORD Ofs)
{
    while (CodeFixupCnt > 0)
    {
        if ((CodeFixups->OfsF & FixOfsMask) >= Ofs) break;
        SetFixEnd();
        CodeFixups++;
        CodeFixupCnt--;
    }
}
//------------------------------------------------------------------------------
//If CodeFixups^ has the Offset=Ofs return it, else - Nil
PFixupRec __fastcall CurFixup(DWORD Ofs)
{
    if (CodeFixupCnt > 0 && ((CodeFixups->OfsF & FixOfsMask) == Ofs))
        return CodeFixups;
    else
        return NULL;
}
//------------------------------------------------------------------------------
//Move CodeFixups to the next fixup, Return true if the next fixup has the Offset<=Ofs
bool __fastcall NextFixup(DWORD Ofs)
{
    if (CodeFixupCnt <= 0) return false;
    SetFixEnd();
    CodeFixups++;
    CodeFixupCnt--;
    if (CodeFixupCnt <= 0) return false;
    if ((CodeFixups->OfsF & FixOfsMask) > Ofs) return false;
    return true;
}
//------------------------------------------------------------------------------
bool __fastcall GetFixupFor(BYTE* CodePtr, DWORD Size, bool StartOk, PFixupRec* Fix)
{
    PFixupRec Fx;
    BYTE F;
    DWORD Ofs;

    *Fix = NULL;
    if (CodePtr + Size > CodeEnd) return false;
    Ofs = CodePtr - CodeStart;
    if (Size == 4)
    {
        SkipFixups(Ofs);
        if (CodePtr < FixUpEnd) return false;
        do
        {
            Fx = CurFixup(Ofs);
            if (!Fx) break;
            F = ((BYTE*)(&Fx->OfsF))[3];
            if (F < fxStart)
            {
                if (*Fix) return false;
                *Fix = Fx;
            }
            else if (F != fxStart || !StartOk) return false;
        } while (NextFixup(Ofs));
        FixUpEnd = CodePtr;
    }
    SkipFixups(Ofs + Size);
    if (CodePtr < FixUpEnd) return false;
    return true;
}
//------------------------------------------------------------------------------
PName __fastcall GetAddrName(int hDef)
{
    TDCURec *D;

    D = GetAddrDef(hDef);
    if (!D) return &NoName;
    return D->GetName();
}
//------------------------------------------------------------------------------
String __fastcall ShowOfsQualifier(int hDef, int Ofs)
{
    TTypeDef *TD;

    TD = GetGlobalTypeDef(hDef);
    if (!TD)
    {
        if (Ofs > 0)
            return Format("+%d", ARRAYOFCONST((Ofs)));
        else if (Ofs < 0)
            return Format("%d", ARRAYOFCONST((Ofs)));
    }
    return "";
}
//------------------------------------------------------------------------------
//This function should check whether DP points to some valid text
int __fastcall TryShowPCharConst(BYTE* DP, DWORD DS)
{
    BYTE *CP, *EP;

    EP = DP + DS;
    CP = DP;
    while (CP < EP && (*CP == 9 || *CP == 0x13 || *CP == 0x10 || *CP >= 0x32)) CP++;
    if (CP >= EP || *CP) return -1;
    if (*DP == 0xE9 && CP == DP + 1) return -1;
    OutLog2("%s", StrConstStr(DP, CP - DP).c_str());
    return CP - DP + 1;
}
//------------------------------------------------------------------------------
bool __fastcall ReportFixup(PFixupRec Fix, int Ofs, bool UseHAl)
{
    TDCURec *D;
    int hDT;
    BYTE *DP;
    DWORD Sz;
    int L;
    String OutS;

    if (!Fix) return false;
    OutLog2("K%d ", ((BYTE*)&Fix->OfsF)[3]);
    D = GetGlobalAddrDef(Fix->Ndx);
    hDT = -1;
    L = -1;
    if (D)
    {
        if (D->InheritsFrom(__classid(TVarDecl)))
            hDT = ((TVarDecl*)D)->hDT;
        else if (UseHAl && Ofs > 0 && D->InheritsFrom(__classid(TProcDecl)))
        {
            DP = GetBlockMem(((TProcDecl*)D)->CodeOfs, ((TProcDecl*)D)->Sz, &Sz);
            if (DP && Ofs <= Sz)
            {
                if (Ofs >= 8)
                    L = ShowStrConst(DP + Ofs - 8, Sz - Ofs + 8, OutS);
                if (L < 0)
                    L = TryShowPCharConst(DP + Ofs, Sz - Ofs);
            }
        }
    }
    if (L > 0) OutLog1(" {");
    OutLog2("%s", GetAddrStr(Fix->Ndx).c_str());
    OutLog2("%s", ShowOfsQualifier(hDT, Ofs).c_str());
    if (L > 0) OutLog1("}");
    return true;
}
//------------------------------------------------------------------------------
void __fastcall ShowPointer(BYTE *DP, String NilStr, String& OutS)
{
    BYTE *V;
    PFixupRec Fix;
    bool VOk;
    PName FxName;
    char HexVal[32];

    OutS = "";
    V = *((BYTE**)DP);
    if (GetFixupFor(DP, 4, true, &Fix) && Fix)
    {
        FxName = GetAddrName(Fix->Ndx);
        OutS += "@";
        OutLog1("@");
        if (!ReportFixup(Fix, (DWORD)V, false))
        {
            if (V)
            {
                sprintf(HexVal, "+$%lX", (DWORD)V);
                OutS += String(HexVal);
                OutLog2("%s", HexVal);
            }
        }
    }
    else if (!V)
    {
        OutS += NilStr;
        OutLog2("%s", NilStr.c_str());
    }
    else
    {
        sprintf(HexVal, "$%.8lX", (DWORD)V);
        OutS += String(HexVal);
        OutLog2("%s", HexVal);
    }
}
//------------------------------------------------------------------------------
char* __fastcall StrLEnd(char* Str, DWORD L)
{
    if (L < strlen(Str)) return Str + L;
    return Str + strlen(Str) - 1;
}
//------------------------------------------------------------------------------
bool __fastcall TPtrDef::ShowRefValue(int Ndx, DWORD Ofs, String& OutS)
{
    char        *DP, *EP;
    TTypeDef    *DT;
    TDCURec     *AR;

    DT = GetGlobalTypeDef(hRefDT);
    if (!DT || !DT->Def || DT->Def->Tag != drChRangeDef) return false;
    AR = GetGlobalAddrDef(Ndx);
    if (!AR || !(AR->InheritsFrom(__classid(TProcDecl)))) return false;
    DP = GetBlockMem(((TProcDecl*)AR)->CodeOfs, ((TProcDecl*)AR)->Sz, (DWORD*)&Sz);
    if (Ofs >= Sz) return false;
    EP = StrLEnd(DP + Ofs, Sz - Ofs);
    if (EP - DP == Sz) return false;
    OutS = StrConstStr(DP + Ofs, EP - (DP + Ofs));
    return true;
}
//------------------------------------------------------------------------------
int __fastcall TPtrDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    OutS = "";
    if (Sz > DS) return -1;
    if (Sz == 4)
    {
        ShowPointer(DP, "Nil", OutS);
        return Sz;
    }
    return TTypeDef::ShowValue(DP, Sz, OutS);
}
//------------------------------------------------------------------------------
void __fastcall TPtrDef::Show(String& OutS)
{
    String S;
    OutS = "";
    TTypeDef::Show(S);
    OutS += S + "^";
    OutLog1("^");
    OutS += ShowTypeDef(hRefDT, NULL);
}
//------------------------------------------------------------------------------
void __fastcall TPtrDef::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    Action(this, hRefDT, IP);
}
//------------------------------------------------------------------------------
String __fastcall TPtrDef::GetRefOfsQualifier(int Ofs)
{
    return "^" + ShowOfsQualifier(hRefDT, Ofs);
}
//------------------------------------------------------------------------------
__fastcall TTextDef::TTextDef() : TTypeDef()
{
}
//------------------------------------------------------------------------------
void __fastcall TTextDef::Show(String& OutS)
{
    String S;
    TTypeDef::Show(S);
    OutS = "text";
    OutLog1("text");
}
//------------------------------------------------------------------------------
__fastcall TFileDef::TFileDef() : TTypeDef()
{
    hBaseDT = ReadUIndex();
}
//------------------------------------------------------------------------------
void __fastcall TFileDef::Show(String& OutS)
{
    String S;
    TTypeDef::Show(S);
    OutS = "file of ";
    OutLog1("file of ");
    OutS += ShowTypeDef(hBaseDT, NULL);
}
//------------------------------------------------------------------------------
void __fastcall TFileDef::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    Action(this, hBaseDT, IP);
}
//------------------------------------------------------------------------------
__fastcall TSetDef::TSetDef() : TTypeDef()
{
    BStart = ReadByte();
    hBaseDT = ReadUIndex();
}
//------------------------------------------------------------------------------
int __fastcall TSetDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    TTypeDef *T;
    int Cnt, K;
    TInt64Rec V0, Lo, Hi;
    bool WasOn, SetOn;
    BYTE B;
    String S;

    OutS = "";
    if (Sz > DS) return -1;
    T = GetGlobalTypeDef(hBaseDT);
    if (!T || !T->InheritsFrom(__classid(TRangeBaseDef))) return -1;
    ((TRangeBaseDef*)T)->GetRange(&Lo, &Hi);
    Lo.Lo = BStart*8;
    Hi.Lo = (BStart + Sz)*8 - 1;
    OutS += "[";
    OutLog1("[");
    Cnt = 0;
    SetOn = false;
    while (Lo.Lo <= Hi.Lo)
    {
        K = (Lo.Lo & 7);
        if (!K)
        {
            B = *DP;
            DP++;
        }
        WasOn = SetOn;
        SetOn = ((B & (1 << K)) != 0);
        if (WasOn != SetOn)
        {
            if (WasOn)
            {
                if (Cnt > 0)
                {
                    OutS += ",";
                    OutLog1(",");
                }
                Cnt++;
                ShowTypeValue(T, (BYTE*)&V0, sizeof(V0), 0, S);
                OutS += S;
                Lo.Lo--;
                if (V0.Lo != Lo.Lo)
                {
                    OutS += "..";
                    OutLog1("..");
                    ShowTypeValue(T, (BYTE*)&Lo, sizeof(Lo), 0, S);
                    OutS += S;
                }
                Lo.Lo++;
            }
            else
                V0.Lo = Lo.Lo;
        }
        Lo.Lo++;
    }
    if (SetOn)
    {
        if (Cnt > 0)
        {
            OutS += ",";
            OutLog1(",");
        }
        Cnt++;
        ShowTypeValue(T, (BYTE*)&V0, sizeof(V0), 0, S);
        OutS += S;
        Lo.Lo--;
        if (V0.Lo != Lo.Lo)
        {
            OutS += "..";
            OutLog1("..");
            ShowTypeValue(T, (BYTE*)&Lo, sizeof(Lo), 0, S);
            OutS += S;
        }
        Lo.Lo++;
    }
    OutS += "]";
    OutLog1("]");
    return Sz;
}
//------------------------------------------------------------------------------
void __fastcall TSetDef::Show(String& OutS)
{
    String S;
    TTypeDef::Show(S);
    OutS = "set of ";
    OutLog1("set of ");
    OutS += ShowTypeDef(hBaseDT, NULL);
}
//------------------------------------------------------------------------------
void __fastcall TSetDef::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    Action(this, hBaseDT, IP);
}
//------------------------------------------------------------------------------
//TArrayDef0
__fastcall TArrayDef0::TArrayDef0(bool IsStr) : TTypeDef()
{
    B1 = ReadByte();
    hDTNdx = ReadUIndex();
    hDTEl = ReadUIndex();
    if (!IsStr && IsMSIL) ReadUIndex();
}
//------------------------------------------------------------------------------
int __fastcall TArrayDef0::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    TTypeDef *T;
    DWORD Rest, ElSz;
    int Cnt;
    String S;

    OutS = "";
    if (Sz > DS) return -1;
    T = GetGlobalTypeDef(hDTEl);
    if (!T) return -1;
    if (T->Def && *((BYTE*)(T->Def)) == drChRangeDef)
    {
        OutS = StrConstStr(DP, Sz);
        OutLog2("%s", OutS.c_str());
        return Sz;
    }
    Rest = Sz;
    ElSz = T->Sz;
    OutS = "(";
    OutLog1("(");
    Cnt = 0;
    while (Rest >= ElSz)
    {
        if (Cnt > 0)
        {
            OutS += ",";
            OutLog1(",");
        }
        if (ShowTypeValue(T, DP, Rest, -1, S) < 0)
        {
            return -1;
        }
        OutS += S;
        Cnt++;
        DP += ElSz;
        Rest -= ElSz;
    }
    OutS += ")";
    OutLog1(")");
    return Sz;
}
//------------------------------------------------------------------------------
void __fastcall TArrayDef0::Show(String& OutS)
{
    OutS = "array";
    OutLog1("array");
    ShowBase();
    OutS += "[";
    OutLog1("[");
    OutS += ShowTypeDef(hDTNdx, NULL);
    OutS += "] of ";
    OutLog1("] of ");
    OutS += ShowTypeDef(hDTEl, NULL);
}
//------------------------------------------------------------------------------
void __fastcall TArrayDef0::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    Action(this, hDTNdx, IP);
    Action(this, hDTEl, IP);
}
//------------------------------------------------------------------------------
//TArrayDef
__fastcall TArrayDef::TArrayDef(bool IsStr) : TArrayDef0(IsStr)
{
}
//------------------------------------------------------------------------------
String __fastcall TArrayDef::GetOfsQualifier(int Ofs)
{
    TTypeDef    *TD;
    int         ElSz;

    TD = GetGlobalTypeDef(hDTEl);
    if (!TD || !Ofs)
        return TTypeDef::GetOfsQualifier(Ofs);
    else
    {
        ElSz = TD->Sz;
        return Format("[%d]%s", ARRAYOFCONST((Ofs / ElSz, ShowOfsQualifier(hDTEl, Ofs % ElSz))));
    }
}
//------------------------------------------------------------------------------
__fastcall TShortStrDef::TShortStrDef() : TArrayDef(true)
{
    if (FVer >= verD2009 && FVer < verK1) CP = ReadUIndex();
}
//------------------------------------------------------------------------------
int __fastcall TShortStrDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    int L;

    OutS = "";
    if (Sz > DS) return -1;
    L = ((PName)DP)->Len;
    if (L >= Sz)
        return TArrayDef::ShowValue(DP, DS, OutS);
    else
    {
        OutS = StrConstStr(DP + 1, L);
        OutLog2("%s", OutS.c_str());
        return Sz;
    }
}
//------------------------------------------------------------------------------
void __fastcall TShortStrDef::Show(String& OutS)
{
    if (Sz == -1)
    {
        OutS = "ShortString";
        OutLog1("ShortString");
    }
    else
    {
        OutS = "String[" + String(Sz - 1) + "]";
        OutLog2("String[%d]", Sz - 1);
    }
    ShowBase();
}
//------------------------------------------------------------------------------
__fastcall TStringDef::TStringDef() : TArrayDef(true)
{
    if (FVer >= verD2009 && FVer < verK1) CP = ReadUIndex();
}
//------------------------------------------------------------------------------
bool __fastcall TStringDef::ShowRefValue(int Ndx, DWORD Ofs, String& OutS)
{
    int         L;
    TDCURec     *AR;
    TProcDecl   *Proc;
    char        *DP;

    if (Ofs < 8) return false;
    AR = GetGlobalAddrDef(Ndx); Proc = (TProcDecl*)AR;
    if (!AR || !(AR->InheritsFrom(__classid(TProcDecl)))) return false;
    DP = GetBlockMem(Proc->CodeOfs, Proc->Sz, (DWORD*)&Sz);
    if (Ofs >= Sz) return false;
    if (Proc->IsUnnamed()) Proc->JustData = true; //Mark the procedure as having no code
    L = ShowStrConst(DP + Ofs - 8, Sz - Ofs + 8, OutS);
    return (L > 0);
}
//------------------------------------------------------------------------------
int __fastcall TStringDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    OutS = "";
    if (Sz > DS) return -1;
    if (Sz == 4)
    {
        ShowPointer(DP, "''", OutS);
        return Sz;
    }
    return TArrayDef::ShowValue(DP, Sz, OutS);
}
//------------------------------------------------------------------------------
void __fastcall TStringDef::Show(String& OutS)
{
    OutS = "String";
    OutLog1("String");
    ShowBase();
}
//------------------------------------------------------------------------------
String __fastcall TStringDef::GetRefOfsQualifier(int Ofs)
{
    TTypeDef    *TD;
    int         ElSz;

    TD = GetGlobalTypeDef(hDTEl);
    if (!TD || !Ofs)
        return TTypeDef::GetRefOfsQualifier(Ofs);
    else
    {
        ElSz = TD->Sz;
        return TTypeDef::GetOfsQualifier(Ofs + ElSz); //Because String is 1-based
    }
}
//------------------------------------------------------------------------------
__fastcall TVariantDef::TVariantDef() : TTypeDef()
{
    if (FVer > verD2) B = ReadByte();
}
//------------------------------------------------------------------------------
void __fastcall TVariantDef::Show(String& OutS)
{
    OutLog1("variant");
    TTypeDef::Show(OutS);
    OutS = "variant";
}
//------------------------------------------------------------------------------
__fastcall TObjVMTDef::TObjVMTDef() : TTypeDef()
{
    hObjDT = ReadUIndex();
    Ndx1 = ReadUIndex();
    if (IsMSIL) ReadUIndex();
}
//------------------------------------------------------------------------------
void __fastcall TObjVMTDef::Show(String& OutS)
{
    TTypeDef::Show(OutS);
    OutS += "class of ";
    OutLog1("class of ");
    OutS += ShowTypeDef(hObjDT, NULL);
}
//------------------------------------------------------------------------------
__fastcall TRecBaseDef::TRecBaseDef() : TTypeDef()
{
}
//------------------------------------------------------------------------------
__fastcall TRecBaseDef::~TRecBaseDef()
{
    FreeDCURecList((TDCURec*)Fields);    
}
//------------------------------------------------------------------------------
void __fastcall TRecBaseDef::ReadFields(BYTE LK)
{
    Tag = ReadTag();
    ReadDeclList(LK, &Fields);
    if (Tag != drStop1) printf("Error: Stop Tag\n");
}
//------------------------------------------------------------------------------
int __fastcall  TRecBaseDef::ShowFieldValues(BYTE* DP, DWORD DS, String& OutS)
{
    int Cnt, Ofs;
    bool Ok;
    TNameDecl *DeclL, *Decl;
    String S;

    OutS = "";
    if (Sz > DS) return -1;
    Cnt = 0;
    Ok = true;
    DeclL = Fields;
    OutS += "(";
    OutLog1("(");

    while (DeclL)
    {
        Decl = DeclL;
        if (Decl->InheritsFrom(__classid(TCopyDecl)))
            Decl = ((TCopyDecl*)Decl)->Base;
        if (Decl->InheritsFrom(__classid(TLocalDecl)) && Decl->GetTag() == arFld)
        {
            if (Cnt > 0)
            {
                OutS += ";";
                OutLog1(";");
            }
            Decl->ShowName(S);
            OutS += S + ":";
            OutLog1(":");
            Ofs = ((TLocalDecl*)Decl)->Ndx;
            if (Ofs < 0 || Ofs > Sz ||
                ShowGlobalTypeValue(((TLocalDecl*)Decl)->hDT, DP + Ofs, Sz - Ofs, false, -1, S) < 0)
            {
                OutS += "?";
                OutLog1("?");
                Ok = false;
            }
            OutS += S;
            Cnt++;
        }
        DeclL = (TNameDecl*)DeclL->Next;
    }
    OutS += ")";
    OutLog1(")");
    if (!Ok)
    {
        TTypeDef::ShowValue(DP, DS, S);
        OutS += S;
    }
    return Sz;
}
//------------------------------------------------------------------------------
void __fastcall TRecBaseDef::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    EnumUsedTypeList(Fields, Action, IP);
}
//------------------------------------------------------------------------------
int __fastcall TRecBaseDef::GetParentType()
{
    return -1;
}
//------------------------------------------------------------------------------
//This procedure is required to find properties corresponding to unnamed fields
TPropDecl* __fastcall TRecBaseDef::GetFldProperty(PNameDecl Fld, int hDT)
{
    TDCURec     *Decl;
    TPropDecl   *Result;

    while (Decl)
    {
        if (Decl->InheritsFrom(__classid(TPropDecl)) && ((TPropDecl*)Decl)->hDT == hDT)
        {
            Result = (TPropDecl*)Decl;
            if (Result->hRead && GetAddrDef(Result->hRead) == Fld) return Result;
            if (Result->hWrite && GetAddrDef(Result->hWrite) == Fld) return Result;
        }
        Decl = Decl->Next;
    }
    return NULL;
}
//------------------------------------------------------------------------------
String __fastcall TRecBaseDef::GetFldOfsQualifier(int Ofs, int TotSize, bool Sorted)
{
    int         FldOfs;
    TDCURec     *DeclL, *Decl;
    TTypeDef    *TD;
    String      FldName;

    if (Ofs >= TotSize) return "";
    DeclL = Fields;
    while (DeclL)
    {
        Decl = DeclL;
        if (Decl->InheritsFrom(__classid(TCopyDecl))) Decl = ((TCopyDecl*)Decl)->Base;
        if (Decl->InheritsFrom(__classid(TLocalDecl)) && ((TLocalDecl*)Decl)->GetTag() == arFld)
        {
            FldOfs = ((TLocalDecl*)Decl)->Ndx;
            if (FldOfs >= 0)
            {
                if (FldOfs <= Ofs)
                {
                    TD = GetGlobalTypeDef(((TLocalDecl*)Decl)->hDT);
                    if (TD && Ofs < FldOfs + TD->Sz)
                    {
                        FldName = PName2String(((TLocalDecl*)Decl)->GetName());
                        if (FldName == "")
                        {
                            Decl = GetFldProperty((TNameDecl*)Decl, ((TLocalDecl*)Decl)->hDT);
                            if (Decl) FldName = PName2String(((TNameDecl*)Decl)->GetName());
                            if (FldName == "") FldName = Format("(:%s)", ARRAYOFCONST((PName2String(TD->GetName()))));
                        }
                        return Format(".%s%s", ARRAYOFCONST((FldName, ShowOfsQualifier(((TLocalDecl*)Decl)->hDT, Ofs - FldOfs))));
                    }
                }
                else
                {
                    if (Sorted) break;
                }
            }
        }
        DeclL = DeclL->Next;
    }
    return "";
}
//------------------------------------------------------------------------------
__fastcall TRecDef::TRecDef() : TRecBaseDef()
{
    BYTE    B1;
    DWORD   X0, X, XX;

    if (FVer >= verD2009 && FVer < verK1) ReadUIndex();
    B2 = ReadByte();
    if (IsMSIL)
    {
        X = ReadUIndex();
        //!Temp Skip interface info - should make it stored in recs too
        ReadClassInterfaces(NULL);
    }
    else if (FVer >= verD2005 && FVer < verK1)
    {
        if (FVer >= verD2006 && FVer < verK1)
        {
            B1 = ReadByte();
            if (FVer >= verD2009 && FVer < verK1) XX = ReadUIndex();
            if (FVer >= verD2010 && FVer < verK1) XX = ReadUIndex();
            X0 = ReadUIndex();
        }
        ReadClassInterfaces(NULL);
        //X = ReadUIndex();     //???????????????
    }
    ReadFields(dlFields);
}
//------------------------------------------------------------------------------
int __fastcall TRecDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    return ShowFieldValues(DP, DS, OutS);
}
//------------------------------------------------------------------------------
//Check whether all the fields start where some previous field ends =>the record could be packed
bool __fastcall ChkIsPacked(TDCURec* L)
{
    const   MaxFieldAlign = 8;
    bool    PkRq, Result;
    TList   *EndL;
    int     Ofs, Sz, SzRem;

    if (!L) return false; //no need for packing
    PkRq = false; //Without PkRq computing almost any record will be packed
    EndL = new TList;
    do
    {
        if (!L->InheritsFrom(__classid(TLocalDecl))) break;
        Ofs = ((TLocalDecl*)L)->Ndx;
        if (Ofs > 0 && EndL->IndexOf((void*)Ofs) < 0) return false;//The field starts at an unknown offset
        Sz = GetTypeSize(((TLocalDecl*)L)->hDT);
        if (Sz < 0) return false;//Unknown data type could be of any size => can't check packing
        SzRem = 0;
        if (Sz > 0)
        {
            SzRem = MaxFieldAlign;
            while (Sz % SzRem > 0)
            {
                SzRem /= 2;
            }
        }
        if (SzRem > 1 && Ofs % SzRem != 0) PkRq = true;
        Ofs += Sz;
        EndL->Add((void*)Ofs);
        L = L->Next;
    } while (L);
    delete EndL;
    return PkRq;
}
//------------------------------------------------------------------------------
//Find the smallest field offset, among the fields before the end of the previous field
int __fastcall GetCaseOfs(TDCURec *L)
{
    int Ofs, Sz, PrevOfs;

    int Result = MAXINT;
    PrevOfs = -1;
    while (L)
    {
        if (!L->InheritsFrom(__classid(TLocalDecl))) return Result;
        Ofs = ((TLocalDecl*)L)->Ndx;
        if (Ofs < PrevOfs)
        {
            if (Ofs < Result) Result = Ofs;
        }
        Sz = GetTypeSize(((TLocalDecl*)L)->hDT);
        if (Sz < 0) Sz = 1; //For unknown data types I suppose that it should take some space
        PrevOfs = Ofs + Sz;
        L = L->Next;
    }
    return Result;
}
//------------------------------------------------------------------------------
//Find 1st field >= OfsRq => 1st case field
TNameDecl** __fastcall GetNoCaseEP(TDCURec** L, int OfsRq)
{
    int Ofs;

    while (*L && (*L)->InheritsFrom(__classid(TLocalDecl)))
    {
        if (((TLocalDecl*)(*L))->Ndx >= OfsRq) return (TNameDecl**)L;
        L = &((*L)->Next);
    }
    return NULL;
}
//------------------------------------------------------------------------------
//Requires: L-case field
//Find the next field with the same or higher (because of alignment) offset
//For example, in D7 this record has the following field offsets:
//TRec = record
//  case integer of
//  0: (A: integer@0);
//  1: (V: byte@0;
//    case integer of
//    0: (B: double@8);
//    1: (C: Byte@4))
//end ;

TNameDecl** __fastcall GetNextEP(TDCURec* L, int OfsRq)
{
    int Ofs, OfsMax, Sz, PrevOfs;

    Sz = GetTypeSize(((TLocalDecl*)L)->hDT);
    if (Sz < 0) Sz = 1;
    OfsMax = ((TLocalDecl*)L)->Ndx + Sz;
    TNameDecl** Result = &(TNameDecl*)(L->Next);
    while (*Result)
    {
        if (!(*Result)->InheritsFrom(__classid(TLocalDecl))) break;
        Ofs = ((TLocalDecl*)(*Result))->Ndx;
        if (Ofs >= OfsRq && Ofs < OfsMax) return Result;
        Result = (TNameDecl**)&((*Result)->Next);
    }
    return NULL;
}
//------------------------------------------------------------------------------
void __fastcall ShowCase(TNameDecl* Start, BYTE SK)
{
    int         CaseOfs, hCase;
    TNameDecl   **EP;
    TNameDecl   *EP0, *CaseP;
    String      S;

    CaseOfs = GetCaseOfs(Start);
    EP = NULL;
    if (CaseOfs < MAXINT)
        EP = GetNoCaseEP((TDCURec**)&Start, CaseOfs);
    if (EP)
    {
        EP0 = *EP;
        *EP = NULL;
    }
    ShowDeclList(dlFields, Start, S);
    if (EP)
    {
        OutLog1("case Integer of\n");
        CaseN = hCase = 0;
        while (1)
        {
            *EP = EP0;
            OutLog2("%d:(", hCase); //The actual case labels and case data type are not stored in DCUs
            CaseP = EP0;
            EP = GetNextEP((TDCURec*)CaseP, CaseOfs);
            if (EP)
            {
                EP0 = *EP;
                *EP = NULL;
            }
            ShowCase(CaseP, SK);
            hCase++; CaseN = hCase;
            OutLog1(")\n");
            if (!EP) break;
        }
    }
}
//------------------------------------------------------------------------------
void __fastcall TRecDef::Show(String& OutS)
{
    String S;

    if (ChkIsPacked(Fields)) OutLog1("packed ");
    OutLog1("record\n");
    TRecBaseDef::Show(S);
    ShowCase(Fields, skPublic);
    OutLog1("end");
    OutS = "";
}
//------------------------------------------------------------------------------
String __fastcall TRecDef::GetOfsQualifier(int Ofs)
{
    return GetFldOfsQualifier(Ofs, Sz, false);
}
//------------------------------------------------------------------------------
__fastcall TProcTypeDef::TProcTypeDef() : TRecBaseDef()
{
    BYTE CK;

    if (FVer > verD2)
        Ndx0 = ReadUIndex();
    hDTRes = ReadUIndex();
    AddSz = 0;
    AddStart = CurPos;
    Tag = ReadTag();
    //99.99% that instead of WHILE it would be enough to use IF
    while (Tag != drEmbeddedProcStart)
    {
        if (Tag == drStop1) return;
        CK = ReadCallKind();
        if (CK == pcRegister)
            Tag = ReadTag();
        else
            CallKind = CK;
      AddSz++;
    }
    ReadFields(dlArgsT);
}
//------------------------------------------------------------------------------
int __fastcall TProcTypeDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    OutS = "";
    if (Sz > DS) return -1;
    if (Sz == 4)
    {
        ShowPointer(DP, "Nil", OutS);
        return Sz;
    }
    return TRecBaseDef::ShowValue(DP, Sz, OutS);
}
//------------------------------------------------------------------------------
bool __fastcall TProcTypeDef::IsProc()
{
    return TypeIsVoid(hDTRes);
}
//------------------------------------------------------------------------------
String __fastcall TProcTypeDef::ProcStr()
{
    if (IsProc())
        return "procedure";
    else
        return "function";
}
//------------------------------------------------------------------------------
void __fastcall TProcTypeDef::ShowDecl(char* Braces, String& OutS)
{
    String S;
    OutS = "";

    TRecBaseDef::Show(S);
    if (Fields)
    {
        OutS += Braces[0];
        OutLog2("%c", Braces[0]);
        ShowDeclList(dlArgsT, Fields, S);
        OutS += S;
        OutS += Braces[1];
        OutLog2("%c", Braces[1]);
    }
    if (!IsProc())
    {
        OutS += ":";
        OutLog1(":");
        OutS += ShowTypeDef(hDTRes, NULL);
    }
    if ((Ndx0 & 0x10) != 0)
    {
        OutS += " of object";
        OutLog1(" of object");
    }
    if (CallKind != pcRegister)
    {
        S = CallKindName[CallKind];
        OutS += " " + S;
        OutLog2(" %s", S.c_str());
    }
}
//------------------------------------------------------------------------------
void __fastcall TProcTypeDef::Show(String& OutS)
{
    String S;

    OutS = ProcStr();
    OutLog2("%s", OutS.c_str());
    ShowDecl("()", S);
    OutS += S;
}
//------------------------------------------------------------------------------
void __fastcall TProcTypeDef::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    TRecBaseDef::EnumUsedTypes(Action, IP);
    if (!IsProc()) Action(this, hDTRes, IP);
}
//------------------------------------------------------------------------------
__fastcall TObjDef::TObjDef() : TRecBaseDef()
{
    B03 = ReadByte();
    hParent = ReadUIndex();
    BFE = ReadByte();
    Ndx1 = ReadIndex();
    B00 = ReadByte();
    ReadFields(dlFields);
}
//------------------------------------------------------------------------------
int __fastcall TObjDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    return ShowFieldValues(DP, DS, OutS);
}
//------------------------------------------------------------------------------
void __fastcall TObjDef::Show(String& OutS)
{
    String S= "";

    OutLog1("object");
    TRecBaseDef::Show(S);
    if (hParent)
        OutLog2("(%s)", ShowTypeName(hParent).c_str());
    ShowDeclList(dlFields, Fields, S);
    OutLog1("end");
}
//------------------------------------------------------------------------------
void __fastcall TObjDef::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    TRecBaseDef::EnumUsedTypes(Action, IP);
    if (hParent) Action(this, hParent, IP);
}
//------------------------------------------------------------------------------
int __fastcall TObjDef::GetParentType()
{
    return hParent;
}
//------------------------------------------------------------------------------
String __fastcall TObjDef::GetOfsQualifier(int Ofs)
{
    String  Result;

    Result = GetFldOfsQualifier(Ofs, Sz, true);
    if (Result != "") return Result;
    if (hParent) return ShowOfsQualifier(hParent, Ofs);
    return TRecBaseDef::GetOfsQualifier(Ofs);
}
//------------------------------------------------------------------------------
__fastcall TClassDef::TClassDef() : TRecBaseDef()
{
    BYTE BX;

    if (FVer >= verD2009 && FVer < verK1) ReadUIndex();
    if (FVer >= verD2006 && FVer < verK1)
        BX = ReadByte();//Some flags
    if (FVer >= verD2009 && FVer < verK1) ReadByte();    //SomeFlags???
    hParent = ReadUIndex();
    InstBaseRTTISz = ReadUIndex();
    InstBaseSz = ReadIndex();
    InstBaseV = ReadUIndex();
    VMCnt = ReadUIndex();
    NdxFE = ReadUIndex();
    Ndx00a = ReadUIndex();
    if (FVer >= verD8 && FVer < verK1)
        B04 = ReadUIndex();
    else
        B04 = ReadByte();
    if (FVer >= verD2010 && FVer < verK1) ReadUIndex();
    if (FVer > verD2)
    {
        ReadBeforeIntf(); //Fo
        ICnt = ReadClassInterfaces(&ITbl);
    }
    ReadFields(dlClass);
}
//------------------------------------------------------------------------------
__fastcall TClassDef::~TClassDef()
{
    if (ITbl) delete[] ITbl;
}
//------------------------------------------------------------------------------
int __fastcall TClassDef::ShowValue(BYTE* DP, DWORD DS, String& OutS)
{
    OutS = "";
    if (Sz > DS) return -1;
    if (Sz == 4)
    {
        ShowPointer(DP, "Nil", OutS);
        return Sz;
    }
    return TRecBaseDef::ShowValue(DP, Sz, OutS);
}
//------------------------------------------------------------------------------
void __fastcall TClassDef::Show(String& OutS)
{
    int i, j;
    String S;

    OutS = "class";
    OutLog1("class");
    if (hParent || ICnt)
    {
        OutS += "(";
        OutLog1("(");
        i = 0;
        if (hParent)
        {
            S = ShowTypeName(hParent);
            OutS += S;
            i++;
        }
        NDXHi = 0;
        for (j = 0; j < ICnt; j++)
        {
            if (i > 0)
            {
                OutS += ",";
                OutLog1(",");
            }
            S = ShowTypeName(ITbl[2*j]);
            OutS += S;
        }
        OutS += ")";
        OutLog1(")");
    }
    OutLog2("VMCnt:%d\n", VMCnt);
    TRecBaseDef::Show(S);
    CaseN = -1;
    ShowDeclList(dlClass, Fields, S);
    OutLog1("end");
}
//------------------------------------------------------------------------------
int __fastcall TClassDef::GetParentType()
{
    return hParent;
}
//------------------------------------------------------------------------------
String __fastcall TClassDef::GetRefOfsQualifier(int Ofs)
{
    String  Result;

    Result = GetFldOfsQualifier(Ofs, InstBaseSz, true);
    if (Result != "") return Result;
    if (hParent) return ShowRefOfsQualifier(hParent, Ofs);
    else return TRecBaseDef::GetRefOfsQualifier(Ofs);
}
//------------------------------------------------------------------------------
void __fastcall TClassDef::ReadBeforeIntf()
{
}
//------------------------------------------------------------------------------
__fastcall TMetaClassDef::TMetaClassDef() : TClassDef()
{
}
//------------------------------------------------------------------------------
void __fastcall TMetaClassDef::ReadBeforeIntf()
{
    hCl = ReadUIndex();
    ReadUIndex(); //Ignore - was always 0
}
//------------------------------------------------------------------------------
__fastcall TInterfaceDef::TInterfaceDef() : TRecBaseDef()
{
    BYTE LK;
    int i, Cnt;

    if (FVer >= verD2009 && FVer < verK1) ReadUIndex();
    hParent = ReadUIndex();
    VMCnt = ReadIndex();
    GUID = (PGUID)ReadMem(16);
    B = ReadByte();
    if ((B & 4) == 0)
        LK = dlInterface;
    else
        LK = dlDispInterface;
    if (FVer >= verD8 && FVer < verK1)
    {
        if (FVer >= verD2010 && FVer < verK1) ReadUIndex();
        /*
        Cnt = ReadUIndex();
        for (i = 0; i < Cnt; i++)
        {
            ReadUIndex();
            ReadUIndex();
        }
        */
        ReadClassInterfaces(NULL);
    }
    ReadFields(LK);
}
//------------------------------------------------------------------------------
void __fastcall TInterfaceDef::Show(String& OutS)
{
    char guid[1024];
    String S, S1;

    OutS = "interface ";
    OutLog1("interface ");
    if (hParent)
    {
        S = ShowTypeName(hParent);
        OutS += "(" + S + ") ";
        OutLog2("(%s)", S.c_str());
    }
    OutLog2("VMCnt:%d\n", VMCnt);
    TRecBaseDef::Show(S);
    sprintf(guid, "['{%8.8lX-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X}']",
        GUID->Data1, GUID->Data2, GUID->Data3, GUID->Data4[0], GUID->Data4[1],
        GUID->Data4[2], GUID->Data4[3], GUID->Data4[4], GUID->Data4[5],
        GUID->Data4[6], GUID->Data4[7]);
    OutS += String(guid);
    OutLog2("%s\n", guid);
    ShowDeclList(dlInterface, Fields, S);
    OutLog1("end");
}
//------------------------------------------------------------------------------
__fastcall TVoidDef::TVoidDef() : TTypeDef()
{
    int     X;

    if (FVer >= verDXE1 && FVer < verK1) X = ReadUIndex();
}
//------------------------------------------------------------------------------
void __fastcall TVoidDef::Show(String& OutS)
{
    String SType;
    OutS = "void";
    OutLog1("void");
    TTypeDef::Show(SType);
    OutS += SType;
}
//------------------------------------------------------------------------------
__fastcall TA6Def::TA6Def() : TDCURec()
{
    Tag = ReadTag();
    ReadDeclList(dlA6, &Args);
    if (Tag != drStop1) printf("Stop Tag\n");
}
//------------------------------------------------------------------------------
__fastcall TA6Def::~TA6Def()
{
    FreeDCURecList(Args);
}
//------------------------------------------------------------------------------
void __fastcall TA6Def::Show(String& OutS)
{
    ShowDeclList(dlA6, Args, OutS);
}
//------------------------------------------------------------------------------
__fastcall TA7Def::TA7Def() : TDCURec()
{
    int     i;

    hClass = ReadUIndex();
    Cnt = ReadUIndex();
    Tbl = new int[Cnt];
    for (i = 0; i < Cnt; i++)
    {
        Tbl[i] = ReadUIndex();
    }
}
//------------------------------------------------------------------------------
__fastcall TA7Def::~TA7Def()
{
    if (Tbl) delete[] Tbl;
}
//------------------------------------------------------------------------------
void __fastcall TA7Def::Show(String& OutS)
{
    char    Sep;
    int     i;

    OutS = "";
    OutLog1("A7");
    Sep = '[';
    for (i = 0; i < Cnt; i++)
    {
        OutLog3("%s#%x", Sep, Tbl[i]);
        Sep = ',';
    }
    OutLog1("]");
}
//------------------------------------------------------------------------------
__fastcall TDelayedImpRec::TDelayedImpRec() : TNameDecl(true)
{
    Inf = ReadULong();
    F = ReadUIndex();
    RefAddrDef(F);
}
//------------------------------------------------------------------------------
void __fastcall TDelayedImpRec::Show(String& OutS)
{
    TNameDecl::Show(OutS);
    OutLog1("B0");
    OutLog3("{%x,#%x}", Inf, F);
    OutS = "";
}
//------------------------------------------------------------------------------
__fastcall TORecDecl::TORecDecl() : TNameDecl(true)
{
    DW = ReadULong();
    B0 = ReadByte();
    B1 = ReadByte();
    Tag = ReadTag();
    ReadDeclList(dlA6, &Args);
    if (Tag != drStop1) printf("Stop Tag\n");
}
//------------------------------------------------------------------------------
__fastcall TORecDecl::~TORecDecl()
{
    FreeDCURecList(Args);
}
//------------------------------------------------------------------------------
void __fastcall TORecDecl::Show(String& OutS)
{
    TNameDecl::Show(OutS);
    OutLog1("ORec");
    ShowDeclList(dlA6, Args, OutS);
    OutS = "";
}
//------------------------------------------------------------------------------
__fastcall TDynArrayDef::TDynArrayDef() : TPtrDef()
{
}
//------------------------------------------------------------------------------
void __fastcall TDynArrayDef::Show(String& OutS)
{
    TTypeDef    *TD;

    ShowBase();
    TD = GetGlobalTypeDef(hRefDT);
    if (TD && TD->InheritsFrom(__classid(TArrayDef0)))
    {
        OutS = "array of";
        OutLog1("array of");
        OutS += ShowTypeDef(((TArrayDef0*)TD)->hDTEl, NULL);
    }
    else
      TPtrDef::Show(OutS);
}
//------------------------------------------------------------------------------
String __fastcall TDynArrayDef::GetRefOfsQualifier(int Ofs)
{
    if (Ofs == -4) return ".Length";
    if (Ofs == -8) return ".RefCnt";
    return TPtrDef::GetRefOfsQualifier(Ofs);
}
//------------------------------------------------------------------------------
__fastcall TTemplateArgDef::TTemplateArgDef() : TTypeDef()
{
    int     i;

    Cnt = ReadUIndex();
    Tbl = new int[Cnt];
    for (i = 0; i < Cnt; i++)
    {
        Tbl[i] = ReadUIndex();
    }
    V5 = ReadUIndex();
}
//------------------------------------------------------------------------------
__fastcall TTemplateArgDef::~TTemplateArgDef()
{
    if (Tbl) delete[] Tbl;
}
//------------------------------------------------------------------------------
void __fastcall TTemplateArgDef::Show(String& OutS)
{
    char    Sep;
    int     i;

    TTypeDef::Show(OutS);
    OutLog1("template arg");
    if (V5 > 0) OutLog2(" #%x", V5);
    if (Cnt > 0)
    {
      Sep = '<';
      for (i = 0; i < Cnt; i++)
      {
          OutLog3("%s#%x", Sep, Tbl[i]);
          Sep = ',';
      }
      OutLog1(">");
    }
}
//------------------------------------------------------------------------------
__fastcall TTemplateCall::TTemplateCall() : TTypeDef()
{
    int     i, X;

    if (FVer >= verDXE1 && FVer<verK1) X = ReadUIndex();
    hDT = ReadUIndex();
    Cnt = ReadUIndex();
    Args = new int[Cnt];
    for (i = 0; i < Cnt; i++)
    {
        Args[i] = ReadUIndex();
    }
    hDTFull = ReadUIndex();
}
//------------------------------------------------------------------------------
__fastcall TTemplateCall::~TTemplateCall()
{
    if (Args) delete[] Args;
}
//------------------------------------------------------------------------------
void __fastcall TTemplateCall::Show(String& OutS)
{
    char    Sep;
    int     i;

    TTypeDef::Show(OutS);
    if (hDTFull) OutS += ShowTypeName(hDTFull);
    OutS += ShowTypeName(hDT);
    Sep = '<';
    for (i = 0; i < Cnt; i++)
    {
        OutS += String(Sep);
        OutLog2("%c", Sep);
        Sep = ',';
        OutS += ShowTypeDef(Args[i], NULL);
    }
    OutLog1(">");
    OutS += ">";
}
//------------------------------------------------------------------------------
void __fastcall TTemplateCall::EnumUsedTypes(TTypeUseAction Action, DWORD *IP)
{
    Action(this, hDT, IP);
}
//------------------------------------------------------------------------------
