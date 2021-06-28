//------------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "Main.h"
//------------------------------------------------------------------------------
extern String RegName[7];
extern String CallKindName[5];
//------------------------------------------------------------------------------
#define     NEW_VERSION
//------------------------------------------------------------------------------
BYTE        ActiveInfo, ActiveScope;
DWORD       *pDumpOffset = 0;
DWORD       *pDumpSize = 0;
TList       *FixupsList = 0;    //List of Fixups
TList       *FieldsList = 0;    //List of Fields ("field")
TList       *PropertiesList = 0;//List of Properties ("property")
TList       *MethodsList = 0;   //List of Methods
TList       *ArgsList = 0;      //("var","val")
TList       *LocalsList = 0;    //List of local vars ("local","local absolute","result")
FILE        *fOut;
FILE        *fLog = 0;
bool        ThreadVar = false;
TList       *ModuleList = 0;    //List of Modules
PMODULEINFO ModuleInfo = 0;
WORD        ModuleID;
TList       *ConstList = 0;     //List of Constants
TList       *TypeList = 0;      //List of Types
TList       *VarList = 0;       //List of Vars
TList       *ResStrList = 0;    //List of ResourceStrings
TList       *ProcList = 0;      //List of Procedures
long        CurOffset = 0L;
int         CaseN = -1;         //Number of current case

TShortString    NoName = {1, "?"};

bool        GenVarCAsVars = false;
int         FVer = 0;
int         FPtrSize = 4;
int         FPlatform = dcuplWin32;
int         FEmbedDepth = 0;
TList       *FEmbeddedTypes = 0; //contains embedding depths
bool        IsMSIL;
int         NDXHi;

BYTE        fxStart = fxStart30;
BYTE        fxEnd = fxEnd30;
BYTE        fxJmpAddr = fxJmpAddr0;

BYTE        *FMemPtr = 0;   //DCUData
BYTE        *CurPos = 0;    //Pointer to DCUData
BYTE        *DefStart = 0;  //Start of definition
BYTE        Tag;
DWORD       Magic;
DWORD       FMemSize;
DWORD       FileSizeH;
DWORD       FT;             //FileTime
DWORD       Stamp;
String      UnitName;
DWORD       Flags;          //UnitFlags
DWORD       UnitPrior;      //UnitPriority

TList       *FUnitImp = 0;  //List of Unit imports
TList       *FTypes = 0;    //List of Types
TList       *FAddrs = 0;    //List of Addrs
int         FhNextAddr;     //Required for ProcAddInfo in FVer>verD8
TStringList *FExportNames = 0;
TNameDecl   *FDecls = 0;
int         FTypeDefCnt = 0;
TList       *FTypeShowStack = 0;

BYTE        *FDataBlPtr = 0;
DWORD       FDataBlSize;
DWORD       FDataBlOfs;
//Fixups
int         FFixupCnt;
TFixupRec   *FFixupTbl = 0;
int         FCodeLineCnt;
//FCodeLineTbl: PCodeLineTbl;
int         FLineRangeCnt;
//FLineRangeTbl: PLineRangeTbl;
int         FLocVarCnt;
PLocVarRec  FLocVarTbl = 0;
bool        FLoaded;
//------------------------------------------------------------------------------
String __fastcall PName2String(PName Name)
{
    return String(Name->Name, Name->Len);
}
//------------------------------------------------------------------------------
bool __fastcall TypeIsVoid(int hDef)
{
    TBaseDef *D;

    if (hDef <= 0 || hDef > FTypes->Count) return true;
    D = (TBaseDef*)FTypes->Items[hDef - 1];
    if (!D) return true;
    return (D->ClassType() == __classid(TVoidDef));
}
//------------------------------------------------------------------------------
PUnitImpRec __fastcall GetUnitImpRec(int hUnit)
{
    return (PUnitImpRec)(FUnitImp->Items[hUnit]);
}
//------------------------------------------------------------------------------
void __fastcall GetUnitImp(int hUnit)
{
    PUnitImpRec UI;

    UI = GetUnitImpRec(hUnit);
}
//------------------------------------------------------------------------------
String __fastcall GetDCURecStr(TDCURec* D, int hDef)
{
    PName   N;
    char    ScopeCh;
    char    *CP;
    char    Pfx[32];
    String Result;

    if (!D)
        N = &NoName;
    else
        N = D->GetName();
    if (!N->Len)
    {
        strcpy(Pfx, "_N%_");
        Result = Format("_%x", ARRAYOFCONST((hDef)));
    }
    else if (N->Name[0] == '.')
    {
        strcpy(Pfx, "_D%_");
        Result = PName2String(N).SubString(2, 255);
    }
    else
    {
        Pfx[0] = 0;
        Result = PName2String(N);
    }
    if (Pfx != "")
    {
        CP = StrScan(Pfx, '%');
        if (CP)
        {
            if (!D)
                ScopeCh = 'N';
            else
            {
                if (D->InheritsFrom(__classid(TTypeDecl)) || D->InheritsFrom(__classid(TTypeDef)))
                    ScopeCh = 'T';
                else if (D->InheritsFrom(__classid(TVarDecl)))
                    ScopeCh = 'V';
                else if (D->InheritsFrom(__classid(TConstDecl)))
                    ScopeCh = 'C';
                else if (D->InheritsFrom(__classid(TProcDecl)))
                    ScopeCh = 'F';
                else if (D->InheritsFrom(__classid(TLabelDecl)))
                    ScopeCh = 'L';
                else if (D->InheritsFrom(__classid(TPropDecl)) || D->InheritsFrom(__classid(TDispPropDecl)))
                    ScopeCh = 'P';
                else if (D->InheritsFrom(__classid(TLocalDecl)))
                    ScopeCh = 'v';
                else if (D->InheritsFrom(__classid(TMethodDecl)))
                    ScopeCh = 'M';
                else if (D->InheritsFrom(__classid(TExportDecl)))
                    ScopeCh = 'E';
                else
                    ScopeCh = 'n';
            }
            do
            {
                *CP = ScopeCh;
                CP = StrScan(CP + 1, '%');
            } while (CP);
        }
        Result = String(Pfx) + Result;
    }
    return Result;
}
//------------------------------------------------------------------------------
void __fastcall ChkListSize(TList* L, int hDef)
{
    if (hDef <= 0) return;
    if (hDef > L->Count)
    {
        if (hDef > L->Capacity)
            L->Capacity = (hDef*3)/2;
        L->Count = hDef;
    }
}
//------------------------------------------------------------------------------
//Two methods against circular references
bool __fastcall RegTypeShow(TBaseDef* T)
{
    if (FTypeShowStack->IndexOf(T) >= 0) return false;
    FTypeShowStack->Add(T);
    return true;
}
//------------------------------------------------------------------------------
void __fastcall UnRegTypeShow(TBaseDef* T)
{
    int C = FTypeShowStack->Count - 1;
    FTypeShowStack->Count = C;
}
//------------------------------------------------------------------------------
void __fastcall AddTypeDef(TTypeDef* TD)
{
    ChkListSize(FTypes, FTypeDefCnt + 1);
    TBaseDef *Def = (TBaseDef*)FTypes->Items[FTypeDefCnt];
    if (Def)
    {
        if (Def->Def)
            OutLog2("Error: Type def #%lX override\n", FTypeDefCnt + 1);
        if (Def->hUnit != TD->hUnit)
            OutLog2("Error: Type def #%lX unit mismatch\n", FTypeDefCnt + 1);
        TD->FName = Def->GetName();
        Def->FName = 0;
        delete Def;
    }
    FTypes->Items[FTypeDefCnt] = TD;
    FTypeDefCnt++;
}
//------------------------------------------------------------------------------
void __fastcall SetListDefName(TList* L, int hDef, int hDecl, PName Name)
{
    TBaseDef* Def;

    if (!L) return;
    if (hDef <= 0) return;
    ChkListSize(L, hDef);
    hDef--;
    Def = (TBaseDef*)(L->Items[hDef]);
    if (!Def)
    {
        Def = new TBaseDef(Name, 0, -1);
        L->Items[hDef] = (void*)Def;
        Def->hDecl = hDecl;
        return;
    }
    if (!Def->FName) Def->FName = Name;
    if (!Def->hDecl) Def->hDecl = hDecl;
}
//------------------------------------------------------------------------------
void __fastcall AddTypeName(int hDef, int hDecl, PName Name)
{
    SetListDefName(FTypes, hDef, hDecl, Name);
}
//------------------------------------------------------------------------------
void __fastcall SkipBlock(int Sz)
{
    CurPos += Sz;
}
//------------------------------------------------------------------------------
BYTE* __fastcall ReadMem(DWORD Sz)
{
    BYTE *Result = CurPos;
    SkipBlock(Sz);
    return Result;
}
//------------------------------------------------------------------------------
int OFFSET = 0;
BYTE __fastcall ReadByte()
{
//!!!
OFFSET = CurPos - FMemPtr;
if (OFFSET >= 0x26CE)
OFFSET = OFFSET;
    BYTE Result = *CurPos;
    CurPos++;
    return Result;
}
//------------------------------------------------------------------------------
void __fastcall ReadByteIfEQ(BYTE V)
{
    BYTE  b = *CurPos;
    if (b != V) return;
    CurPos++;
}
//------------------------------------------------------------------------------
int __fastcall ReadByteFrom(bool V)
{
    BYTE  b = *CurPos;
    if (V)  //FVer >= verD2010
    {
        if (b != 0 && b != 1 && b != 2 && b != 4 && b != 8 && b != 16 && b != 24 && b != 32 && b != 33 && b != 97 && b != 128 && b != 132) return -1;
    }
    else    //FVer < verD2010
    {
        if (b != 0 && b != 2 && b != 4 && b != 8 && b != 16 && b != 24 && b != 32 && b != 33 && b != 97 && b != 128 && b != 132) return -1;
    }
    CurPos++;
    return b;
}
//------------------------------------------------------------------------------
BYTE __fastcall ReadTag()
{
    DefStart = CurPos;
    return ReadByte();
}
//------------------------------------------------------------------------------
DWORD __fastcall ReadULong()
{
    DWORD Result = *((DWORD*)CurPos);
    CurPos += 4;
    return Result;
}
//------------------------------------------------------------------------------
String __fastcall ReadStr()
{
    String Res;

    BYTE Len = ReadByte();
    Res = String((char*)CurPos, Len); CurPos += Len;
    return Res;
}
//------------------------------------------------------------------------------
//Return pointer to ShortString (Pascal)
PName __fastcall ReadName()
{
    PName Result = (PName)CurPos;
    SkipBlock(ReadByte());
    return Result;
}
//------------------------------------------------------------------------------
String __fastcall ReadNDXStr()
//Was observed only in drConstAddInfo records of MSIL
{
  int       L;
  String    Res;

  L = ReadUIndex();
  Res = String((char*)CurPos, L); CurPos += L;
  return Res;
}
//------------------------------------------------------------------------------
typedef struct
{
    BYTE    B;
    int     L;
} TR4;

int __fastcall ReadUIndex()
{
int         Result;
BYTE        B[5];
WORD        *W = (WORD*)B;
DWORD       *L = (DWORD*)B;
TR4         *R4 = (TR4*)B;

    NDXHi = 0;
    B[0] = ReadByte();
    if ((B[0] & 1) == 0)
        Result = (DWORD)(B[0] >> 1);
    else
    {
        B[1] = ReadByte();
        if ((B[0] & 2) == 0)
            Result = (DWORD)(*W >> 2);
        else
        {
            B[2] = ReadByte();
            B[3] = 0;
            if ((B[0] & 4) == 0)
                Result = *L >> 3;
            else
            {
                B[3] = ReadByte();
                B[4] = 0;
                if ((B[0] & 8) == 0)
                    Result = *L >> 4;
                else
                {
                    B[4] = ReadByte();
                    Result = (DWORD)R4->L;
                    if (FVer > 3 && ((B[0] & 0xF0) != 0))
                        NDXHi = ReadULong();
                }
            }
        }
    }
    return Result;
}
//------------------------------------------------------------------------------
typedef struct
{
    WORD    W;
    short   i;
} TRL;

int __fastcall ReadIndex()
{
int         Result;
BYTE        B[8];
char        *SB = B;
short       *W = (short*)B;
int         *L = (int*)B;
TR4         *R4 = (TR4*)B;
TRL         *RL = (TRL*)B;

    B[0] = ReadByte();
    if ((B[0] & 1) == 0)
    {
        Result = *SB;
        __asm   sar [Result], 1
    }
    else
    {
        B[1] = ReadByte();
        if ((B[0] & 2) == 0)
        {
            Result = *W;
            __asm   sar [Result], 2
        }
        else
        {
            B[2] = ReadByte();
            B[3] = 0;
            if ((B[0] & 4) == 0)
            {
                RL->i = (char)(B[2]);
                Result = *L;
                __asm   sar [Result], 3
            }
            else
            {
                B[3] = ReadByte();
                if ((B[0] & 8) == 0)
                {
                    Result = *L;
                    __asm   sar [Result], 3
                }
                else
                {
                    B[4] = ReadByte();
                    Result = R4->L;
                    if (FVer > 3 && ((B[0] & 0xF0) != 0))
                    {
                        NDXHi = ReadULong();
                        return Result;
                    }
                }
            }
        }
    }
    if (Result < 0)
        NDXHi = -1;
    else
        NDXHi = 0;
    return Result;
}
//------------------------------------------------------------------------------
void __fastcall ReadIndex64(PInt64Rec Res)
{
    Res->Lo = ReadIndex();
    Res->Hi = NDXHi;
}
//------------------------------------------------------------------------------
void __fastcall ReadUIndex64(PInt64Rec Res)
{
    Res->Lo = ReadUIndex();
    Res->Hi = NDXHi;
}
//------------------------------------------------------------------------------
String __fastcall NDXToStr(int NDXLo)
{
    char buf[256];

    if (!NDXHi)
        sprintf(buf, "$%lX", NDXLo);
    else if (NDXHi == -1)
        sprintf(buf, "-$%lX", -NDXLo);
    else if (NDXHi < 0)
        sprintf(buf, "-$%lX%08lX", -NDXHi - 1, -NDXLo);
    else
        sprintf(buf, "$%lX%08lX", NDXHi, NDXLo);
    return String(buf);
}
//------------------------------------------------------------------------------
#ifdef LINUX
const char AlterSep = '\\';
#else
const char AlterSep = '/';
#endif
String __fastcall ExtractFileNameAnySep(String FN)
{
    String Result = ExtractFileName(FN);
    char* CP = StrRScan(Result.c_str(), AlterSep);
    if (!CP)
        return Result;
    else
        return StrPas(CP + 1);
}
//------------------------------------------------------------------------------
//In D10 some codes were changed, we'll try to move them back
BYTE __fastcall FixTag(BYTE Tag)
{
BYTE    Result = Tag;

    if (FVer >= verD2006 && FVer < verK1)
    {
        if (Result >= 0x2D && Result <= 0x36)
        {
            Result--;
            if (Result < 0x2D)
                Result = 0x36; //This code could be wrong, but the overloaded value of $2D should be moved somewhere
        }
    }
    return Result;
}
//------------------------------------------------------------------------------
void __fastcall RegisterEmbeddedTypes(PNameDecl *Embedded, int Depth)
{
    PDCURec             *DP;
    PDCURec             D;
    PTypeDecl           TD;
    PEmbeddedTypeInf    TI;

    DP = (PDCURec*)Embedded;
    while (1)
    {
        D = *DP;
        if (D == 0) return;
        if (D->InheritsFrom(__classid(TTypeDecl))) //The effect was noticed only for types
        {
            TD = (PTypeDecl)D;
            if (FEmbeddedTypes == 0) FEmbeddedTypes = new TList;
            ChkListSize(FEmbeddedTypes, TD->hDef);
            TI = (PEmbeddedTypeInf)(FEmbeddedTypes->Items[TD->hDef - 1]);
            if (TI == 0)
            {
                TI = new TEmbeddedTypeInf;
                TI->TD = 0;
                TI->Depth = 0;
                FEmbeddedTypes->Items[TD->hDef - 1] = TI;
            }
            else if (TI->TD->hDecl > TD->hDecl)
                TI->TD = 0;
            if (TI->TD == 0)
            {
                TI->TD = TD;
                TI->Depth = Depth;
            }
            *DP = D->Next;
        }
        else
            DP = &(D->Next);
    }
}
//------------------------------------------------------------------------------
typedef struct
{
    TList   *EmbL;
    TList   *EmbeddedTypes;
} TBindEmbeddedTypeInf, *PBindEmbeddedTypeInf;

void _fastcall BindEmbeddedType(PDCURec UseRec, int hDT, DWORD* IP)
{
    PBindEmbeddedTypeInf    betf = (PBindEmbeddedTypeInf)IP;
    int                     D;
    PEmbeddedTypeInf        TI;
    PTypeDecl               TD;
    PProcDecl               PD;

    if (hDT <= 0 || hDT > betf->EmbeddedTypes->Count) return;
    TI = (PEmbeddedTypeInf)betf->EmbeddedTypes->Items[hDT - 1];
    if (TI == 0)return;
    if (TI->Depth > betf->EmbL->Count) return;
    betf->EmbeddedTypes->Items[hDT - 1] = 0;
    PD = (PProcDecl)betf->EmbL->Items[TI->Depth - 1];
    TD = TI->TD;
    TD->Next = PD->Locals;
    PD->Locals = TD;
    delete TI;
    TD->EnumUsedTypes(BindEmbeddedType, IP);
}
//------------------------------------------------------------------------------
void __fastcall EnumUsedTypeList(PDCURec L, TTypeUseAction Action, DWORD* IP)
{
    while (L != 0)
    {
        L->EnumUsedTypes(Action, IP);
        L = L->Next;
    }
}
//------------------------------------------------------------------------------
TBindEmbeddedTypeInf    BindEmbeddedTypeInf;
void __fastcall CheckProcedures(PDCURec D)
{
    while (D != 0)
    {
        if (D->InheritsFrom(__classid(TProcDecl)))
        {
            BindEmbeddedTypeInf.EmbL->Add(D);
            EnumUsedTypeList(((PProcDecl)D)->Locals, BindEmbeddedType, (DWORD*)&BindEmbeddedTypeInf);
            EnumUsedTypeList(((PProcDecl)D)->Embedded, BindEmbeddedType, (DWORD*)&BindEmbeddedTypeInf);
            CheckProcedures(((PProcDecl)D)->Embedded);
            BindEmbeddedTypeInf.EmbL->Count = BindEmbeddedTypeInf.EmbL->Count - 1;
        }
        D = D->Next;
    }
}

void __fastcall BindEmbeddedTypes()
{
    int                 i;
    PEmbeddedTypeInf    TI;

    if (FEmbeddedTypes == 0) return;
    BindEmbeddedTypeInf.EmbL = new TList;
    BindEmbeddedTypeInf.EmbeddedTypes = FEmbeddedTypes;
    CheckProcedures(FDecls);
    delete BindEmbeddedTypeInf.EmbL;
    for (i = FEmbeddedTypes->Count - 1; i >= 0; i--)
    {
        TI = (PEmbeddedTypeInf)FEmbeddedTypes->Items[i];
        if (TI == 0) continue;
        TI->TD->Next = FDecls;
        FDecls = TI->TD;
        delete TI;
    }
    delete FEmbeddedTypes;
    FEmbeddedTypes = 0;
}
//------------------------------------------------------------------------------
BYTE __fastcall ReadCallKind()
{
    BYTE Result = pcRegister;
    if (Tag >= arCDecl && Tag <= arSafeCall)
    {
        Result = Tag - arCDecl + 1;
        Tag = ReadTag();
    }
    return Result;
}
//------------------------------------------------------------------------------
int __fastcall ReadClassInterfaces(int** PITbl)
{
    int     i, j, hIntf, MCnt, N, hMember;
    int     X1, X2, X3;
    int     *ITbl;
    int     Cnt;

    int Result = ReadIndex();
    if (Result <= 0) return Result;
    ITbl = NULL;
    if (PITbl)
    {
        ITbl = new int[Result*2];
        *PITbl = ITbl;
    }
    for (int i = 0; i < Result; i++)
    {
        hIntf = ReadUIndex();
        MCnt = ReadUIndex();
        if (ITbl)
        {
            ITbl[2*i] = hIntf;
            ITbl[2*i+1] = MCnt;
        }
        if (IsMSIL)
        {
            for (j = 0; j < MCnt; j++)
            {
                N = ReadUIndex();
                hMember = ReadUIndex();
            }
        }
        if (FVer >= verD2006 && FVer < verK1)
        {
            X1 = ReadIndex();   //ReadUIndex?
            if (FVer >= verD2010 && FVer < verK1)
            {
                Cnt = ReadUIndex();
                for (j = 0; j < Cnt; j++)
                {
                    ReadUIndex();   //+4
                    ReadUIndex();   //+8
                    ReadByte();     //=1
                    ReadName();
                }
            }
            X2 = ReadUIndex();
            if (FVer >= verD2010 && FVer < verK1)
                X3 = ReadUIndex();
        }
    }
    return Result;
}
//------------------------------------------------------------------------------
TList   *FSrcFiles = 0;
void __fastcall ShowSourceFiles()
{
PSrcFileRec     SFR;

    ModuleInfo->Name = UnitName;
#ifdef SHOW
    OutLog2("Unit %s\n", UnitName.c_str());
    if (FVer > verD2)
    {
        OutLog2("Flags: %lX\n", Flags);
        if (FVer > verD3)
            OutLog2("Priority %lX\n", UnitPrior);
    }
    OutLog1("Source files:\n");
    for (int n = 0; n < FSrcFiles->Count; n++)
    {
        SFR = (PSrcFileRec)FSrcFiles->Items[n];
        BYTE T = SFR->Def->Tag;
        switch (T)
        {
        case drSrc:
            OutLog1("src");
            break;
        case drRes:
            OutLog1("res");
            break;
        case drObj:
            OutLog1("obj");
            break;
        case drAsm:
            OutLog1("asm");
            break;
        }
        OutLog2(" %s\n", String(SFR->Def->Name.Name, SFR->Def->Name.Len).c_str());
    }
#endif
}
//------------------------------------------------------------------------------
void __fastcall ReadSourceFiles()
{
int             F;
PSrcFileRec     SFR, SFRMain = 0;

    FSrcFiles = new TList;
    while (Tag == drSrc || Tag == drRes || Tag == drObj || Tag == drAsm || (FVer >= verD2010 && FVer < verK1 && Tag == drUnitInlineSrc))
    {
        SFR = new TSrcFileRec;
        SFR->Def = (PNameDef)DefStart;
        ReadName();
        ReadULong();    //FileTime
        F = ReadUIndex();
        if (!F) SFRMain = SFR;
        SFR->Ndx = F;
        FSrcFiles->Add((void*)SFR);
        Tag = ReadTag();
    }
    if (!FSrcFiles->Count) return;
    if (!SFRMain) SFRMain = (PSrcFileRec)FSrcFiles->Items[0];
    UnitName = ExtractFileNameAnySep(String(SFRMain->Def->Name.Name, SFRMain->Def->Name.Len));
    char *CP = StrRScan(UnitName.c_str(), '.');
    UnitName.SetLength(CP - UnitName.c_str());
}
//------------------------------------------------------------------------------
void __fastcall ReadUses(BYTE TagRq)
{
char            Ch;
int             hUses, hPack, hImp, hUnit;
int             ndx, ImpBase, ImpBase0, ImpReBase;
DWORD           RTTISz;
int             L, L1, L2;
PName           UseName, ImpN;
PUnitImpRec     U;
TBaseDef        *TR, *AR;
TImpDef         *IR;

    hUses = 0;
    ImpBase = 0;
    while (Tag == TagRq)
    {
        UseName = ReadName();
        U = new TUnitImpRec;
        memset((void*)U, 0, sizeof(TUnitImpRec));
        U->Name = UseName;
        Ch = '?';
        switch (TagRq)
        {
        case drUnit1:   //0x65
            Ch = 'U';
            U->Flags = ufImpl;
            break;
        case drDLL:     //0x68
            Ch = 'D';
            U->Flags = ufDLL;
            break;
        }
        hUnit = FUnitImp->Count;
        FUnitImp->Add((void*)U);
        hPack = 0;
        if (TagRq != drDLL && FVer >= verD8 && FVer < verK1)
            hPack = ReadUIndex();
            
        if (FVer >= verD2006 && FVer < verK1)
            L = ReadUIndex();
        else
            L = ReadULong();

        if ((FVer == verD7 && FVer < verK1) || (FVer >= verD8 && FVer < verK1 && TagRq == drDLL))
            L1 = ReadULong();
        if (FVer >= verD2009 && FVer < verK1)
            L2 = ReadUIndex();
            
        hImp = 0;
        IR = new TImpDef(Ch, UseName, L, NULL, hUnit);
        U->Ref = IR;
        ImpBase0 = ImpBase;
        ImpBase = AddAddrDef(IR);

        if (hPack > 0 && FVer < verD2009)
            RefAddrDef(hPack); //Reserve index for unit package number

        while (1)
        {
            Tag = ReadTag();
            if (Tag == drImpType || Tag == drImpTypeDef)
            {
                if (TagRq != drDLL) //0x68
                {
                    Ch = 'T';
                    ImpN = ReadName();
                    if (Tag == drImpTypeDef) RTTISz = ReadUIndex();
                    L = ReadULong();
                    if (Tag == drImpTypeDef)
                        TR = new TImpTypeDefRec(ImpN, L, RTTISz, NULL, hUnit);
                    else
                        TR = new TImpDef('T', ImpN, L, NULL, hUnit);
                    FTypes->Add((void*)TR);
                    TR->hDecl = AddAddrDef(TR);
                    ndx = FTypes->Count;
                    FTypeDefCnt = ndx;
                }
            }
            else if (Tag == drImpVal)
            {
                Ch = 'A';
                ImpN = ReadName();
                L = ReadULong();
                if (TagRq != drDLL)
                    AR = new TImpDef('A', ImpN, L, NULL, hUnit);
                else
                    AR = new TDLLImpRec(ImpN, L, NULL, hUnit);
                ndx = AddAddrDef(AR);
                TR = AR;
            }
            else if (Tag == drStop2)
            {
                L = -1;
                if (FVer >= verD8 && FVer < verK1)
                    L = ReadULong();
                continue;
            }
            else if (Tag == drConstAddInfo)
            {
                if (!IsMSIL) break;
                if (hImp) printf("Warning: ConstAddInfo encountered for %s in subrecord #%d\n", UseName, hImp);
                ImpReBase = ReadConstAddInfo(NULL);
                continue;
            }
            else
                break;
            hImp++;
        }

        if (Tag != drStop1) printf("Error: Unexpected tag: %lX\n", Tag);   //0x63
        hUses++;
        Tag = ReadTag();
        if (Tag == drProcAddInfo)   //0x9E
        {
            if (FVer < verD7 || FVer >= verK1) break;
            hImp = ReadIndex();
            SetProcAddInfo(hImp);
            Tag = ReadTag();
        }
    }
}
//------------------------------------------------------------------------------
void __fastcall ShowUses(String PfxS, BYTE FRq)
{
    int i, Cnt;
    PUnitImpRec U;
    String name;

    Cnt = 0;
    for (i = 0; i < FUnitImp->Count; i++)
    {
        U = (PUnitImpRec)FUnitImp->Items[i];
        if (FRq != U->Flags) continue;
        name = String(U->Name->Name, U->Name->Len);
        if (ModuleInfo->UsesList->IndexOf(name) == -1)
            ModuleInfo->UsesList->Add(name);
        if (Cnt > 0)
            OutLog1(",");
        else
            OutLog2("%s ", PfxS.c_str());
        OutLog2("%s", name.c_str());
        Cnt++;
    }
    if (Cnt > 0) OutLog1(";");
    OutLog1("\n");
}
//------------------------------------------------------------------------------
void __fastcall SetDeclMem(int hDef, DWORD Ofs, DWORD Sz)
{
    TDCURec *D;
    DWORD Base, Rest;

    D = (TDCURec*)FAddrs->Items[hDef - 1];
    Base = 0;
    while (D)
    {
        if (D->InheritsFrom(__classid(TProcDecl)))
            ((TProcDecl*)D)->AddrBase = Base;
        Rest = D->SetMem(Ofs + Base, Sz - Base);
        if ((int)Rest <= 0) break;
        Base = Sz - Rest;
        D = D->Next;
    }
}
//------------------------------------------------------------------------------
void    LoadFixups()
{
    int i;
    DWORD CurOfs, PrevDeclOfs, dOfs;
    BYTE B1;
    TFixupRec *FP;
    int hPrevDecl;

    FFixupCnt = ReadUIndex();
    FFixupTbl = new TFixupRec[FFixupCnt]; memset(FFixupTbl, 0, FFixupCnt * sizeof(TFixupRec));  
    CurOfs = 0;
    FP = FFixupTbl;
    for (i = 0; i < FFixupCnt; i++)
    {
        dOfs = ReadUIndex();
        CurOfs += dOfs;
        B1 = ReadByte();
        FP->OfsF = (CurOfs & FixOfsMask) | (B1 << 24);
        FP->Ndx = ReadUIndex();
        FP++;
    }
    //After loading fixups set the memory sizes of CBlock parts
    CurOfs = 0;
    FP = FFixupTbl;
    hPrevDecl = 0;
    PrevDeclOfs = 0;
    for (i = 0; i < FFixupCnt; i++)
    {
        CurOfs = (FP->OfsF & FixOfsMask);
        B1 = ((BYTE*)(&FP->OfsF))[3];
//!!!
//if (B1 != 12 && B1 != 1 && B1 != 2 && B1 != 3 && B1 != 5 && B1 != 13)
//B1 = B1;
        if (B1 == fxStart || B1 == fxEnd)
        {
            if (hPrevDecl > 0)
                SetDeclMem(hPrevDecl, PrevDeclOfs, CurOfs - PrevDeclOfs);
            hPrevDecl = FP->Ndx;
            PrevDeclOfs = CurOfs;
            FDataBlOfs = CurOfs;
        }
        FP++;
    }
}
//------------------------------------------------------------------------------
void LoadCodeLines()
{
    int i, CurL, dL;
    DWORD CurOfs, dOfs;

    DWORD FCodeLineCnt = ReadUIndex();
    CurL = 0;
    CurOfs = 0;
    for (i = 0; i < FCodeLineCnt; i++)
    {
        dL = ReadIndex();
        dOfs = ReadUIndex();
        CurOfs += dOfs;
        CurL += dL;
    }
}
//------------------------------------------------------------------------------
void LoadLineRanges()
{
    int i;
    int hFile, Num;
    DWORD Line0, LineNum;

    DWORD FLineRangeCnt = ReadUIndex();
    Num = 0;
    for (int i = 0; i < FLineRangeCnt; i++)
    {
        Line0 = ReadUIndex();
        LineNum = ReadUIndex();
        Num += LineNum;
        hFile = ReadUIndex();
    }
}
//------------------------------------------------------------------------------
void LoadStrucScope()
{
    int Cnt, i;

    Cnt = ReadUIndex();
    for (i = 0; i < Cnt*5; i++)
        ReadUIndex();
}
//------------------------------------------------------------------------------
void LoadSymbolInfo()
{
    int Cnt, NPrimary, i, j;
    int hSym;
    int hMember; //for symbols - type members, else - 0
    int Sz;
    int hDef;//index of symbol definition in the L array

    Cnt = ReadUIndex();
    NPrimary = ReadUIndex();
    for (i = 0; i < Cnt; i++)
    {
        hSym = ReadUIndex();
        hMember = ReadUIndex();
        Sz = ReadUIndex();
        hDef = ReadUIndex();
        for (j = 0; j < Sz; j++)
            ReadUIndex();
    }
}
//------------------------------------------------------------------------------
void LoadLocVarTbl()
{
    int i;
    PLocVarRec LR;

    FLocVarCnt = ReadUIndex();
    FLocVarTbl = new TLocVarRec[FLocVarCnt]; memset(FLocVarTbl, 0, FLocVarCnt * sizeof(FLocVarCnt));
    LR = FLocVarTbl;
    for (i = 0; i < FLocVarCnt; i++)
    {
        LR->sym = ReadUIndex();
        LR->ofs = ReadUIndex();
        LR->frame = ReadIndex();
        LR++;
    }
}
//------------------------------------------------------------------------------
int __fastcall AddAddrDef(TDCURec* ND)
{
    int Result;

    if (FhNextAddr > 0)
    {
        Result = FhNextAddr;
        FAddrs->Items[Result - 1] = (void*)ND;
        if (FVer >= verDXE1 && FVer < verK1)
            FhNextAddr = -1;
        else
            FhNextAddr++;
        return Result;
    }
    FAddrs->Add((void*)ND);
    return FAddrs->Count;
}
//------------------------------------------------------------------------------
void __fastcall RefAddrDef(int V)
{
    if (V > FAddrs->Count)
    {
        if (V != FAddrs->Count + 1)
            printf("Error: Unexpected forward hDecl=0x%x<>0x%x", V, FAddrs->Count + 1);
        FAddrs->Add(NULL);
    }
}
//------------------------------------------------------------------------------
TDCURec* __fastcall GetAddrDef(int hDef)
{
    if (hDef <= 0 || hDef > FAddrs->Count) return NULL;
    return (TDCURec*)FAddrs->Items[hDef-1];
}
//------------------------------------------------------------------------------
String __fastcall GetAddrStr(int hDef)
{
    return GetDCURecStr(GetAddrDef(hDef), hDef);
}
//------------------------------------------------------------------------------
TTypeDef* __fastcall GetLocalTypeDef(int hDef)
{
    //The type should be from this unit
    TBaseDef* D;

    D = GetTypeDef(hDef);
    if (D->InheritsFrom(__classid(TTypeDef)))
        return (TTypeDef*)D;
    else //TImpDef
        return NULL;
}
//------------------------------------------------------------------------------
TTypeDef* __fastcall GetGlobalTypeDef(int hDef)
{
    TBaseDef* D;
    int hUnit;
    PName N;

    D = GetTypeDef(hDef);
    while (1)
    {
        if (!D) return NULL;
        if (D->InheritsFrom(__classid(TTypeDef))) break;
        if (!D->InheritsFrom(__classid(TImpDef))) return NULL;
        if (D->InheritsFrom(__classid(TImpTypeDefRec)))
        {
            hUnit = ((TImpTypeDefRec*)D)->hImpUnit;
            N = ((TImpTypeDefRec*)D)->ImpName;
        }
        else
        {
            hUnit = ((TImpDef*)D)->hUnit;
            N = ((TImpDef*)D)->GetName();
        }
        //GetUnitImp(hUnit);
        return NULL;
    }
    return (TTypeDef*)D;
}
//------------------------------------------------------------------------------
int __fastcall GetTypeSize(int hDef)
{
    TTypeDef *T;

    T = GetGlobalTypeDef(hDef);
    if (!T) return -1;
    return T->Sz;
}
//------------------------------------------------------------------------------
void __fastcall SetProcAddInfo(int V)
{
    if (V == -1)
        FhNextAddr = 0;
    if (V >= 1 && FVer >= verD7)
        FhNextAddr = V;
}
//------------------------------------------------------------------------------
void __fastcall FreeDCURecList(TDCURec* L)
{
    TDCURec *Tmp;

    while (L)
    {
        Tmp = L;
        L = L->Next;
        delete Tmp;
    }
}
//------------------------------------------------------------------------------
TDCURec* __fastcall GetDCURecListEnd(TDCURec* L)
{
    TDCURec* Result = L;
    while (Result)
        Result = Result->Next;
    return Result;
}
//------------------------------------------------------------------------------
void __fastcall SetExportNames(TNameDecl* Decl)
{
    FExportNames = new TStringList;
    FExportNames->Sorted = true;
    FExportNames->Duplicates = dupAccept;
    while (Decl)
    {
        if (Decl->InheritsFrom(__classid(TNameFDecl)) && Decl->IsVisible(dlMain))
            FExportNames->AddObject(PName2String(Decl->GetName()), Decl);
            
        Decl = (TNameDecl*)Decl->Next;
    }
}
//------------------------------------------------------------------------------
TTypeDef* __fastcall GetTypeDef(int hDef)
{
    if (hDef <= 0 || hDef > FTypes->Count) return NULL;
    return (TTypeDef*)FTypes->Items[hDef - 1];
}
//------------------------------------------------------------------------------
void __fastcall SetEnumConsts(TNameDecl** Decl)
{
    TConstDecl *LastConst;
    int ConstCnt;
    TNameDecl *D;
    TNameDecl **DeclP, **LastConstP;
    TTypeDef *TD;
    TEnumDef *Enum;
    TList *NT;

    DeclP = Decl;
    LastConstP = NULL;
    LastConst = NULL;
    ConstCnt = 0;
    while (*DeclP)
    {
        D = *DeclP;
        if (D->InheritsFrom(__classid(TConstDecl)))
        {
            if (LastConst && LastConst->hDT == ((TConstDecl*)D)->hDT)
            {
                ConstCnt++;
            }
            else
            {
                LastConstP = DeclP;
                LastConst = (TConstDecl*)D;
                ConstCnt = 1;
            }
        }
        else
        {
            if (D->InheritsFrom(__classid(TTypeDecl)) && LastConst)
            {
                TD = GetTypeDef(((TTypeDecl*)D)->hDef);
                if (TD->InheritsFrom(__classid(TEnumDef)))
                {
                    Enum = (TEnumDef*)TD;
                    NT = new TList;
                    NT->Capacity = ConstCnt;
                    *LastConstP = D;
                    *DeclP = NULL;
                    while (LastConst)
                    {
                        NT->Add((void*)LastConst);
                        LastConst = (TConstDecl*)(LastConst->Next);
                    }
                    Enum->NameTbl = NT;
                }
            }
            LastConst = NULL;
            ConstCnt = 0;
        }
        DeclP = &(TNameDecl*)(D->Next);
    }
}
//------------------------------------------------------------------------------
void __fastcall FillProcLocVarTbls()
{
    int i, iStart;
    PLocVarRec LVP;
    TProcDecl *Proc;
    TDCURec *D;
    bool wasProc, isProc;
    int f;

    if (!FLocVarTbl) return;
    LVP = FLocVarTbl;
    isProc = false;
    Proc = NULL;
    iStart = 0;
    for (i = 0; i < FLocVarCnt; i++)
    {
        wasProc = isProc;
        f = LVP->frame;
        D = NULL;
        isProc = false;
        if (!wasProc && LVP->sym)
        {
            D = GetAddrDef(LVP->sym);
            isProc = (D->InheritsFrom(__classid(TProcDecl)));
            if (isProc)
            {
                if (Proc)
                {
                    Proc->FProcLocVarTbl = &FLocVarTbl[iStart];
                    Proc->FProcLocVarCnt = i - iStart;
                }
                Proc = (TProcDecl*)D;
                iStart = i;
            }
        }
        LVP++;
    }
    if (Proc)
    {
        Proc->FProcLocVarTbl = &FLocVarTbl[iStart];
        Proc->FProcLocVarCnt = FLocVarCnt - iStart;
    }
}
//------------------------------------------------------------------------------
int __fastcall ReadConstAddInfo(TNameDecl* LastProcDecl)
{
    bool    brk = false;
    BYTE    Tag, caiStop, b, *cPos;
    int     hDef, hDef1, hDef2, hDef3, hDef4, hDef5, hDT, F, IP, i, j;
    int     V1, V2, V3, V4, V5, InlineMask;
    DWORD   Len, Len1, V, hUnit;
    DWORD	W1, W2;
    int     hDef11, hDef12, hDef13, hDef14, hDef15;
    int     IP2, Z;
    String  S;

    int Result = -1;
    if (FVer <= verD7 || FVer >= verK1)
    {
        ReadByte();
        ReadUIndex();
        ReadByte();
        ReadByte();
        return Result;
    }

    caiStop = 0xD;
    if (FVer >= verD2005)
    {
        caiStop = 0xF;
        if (FVer >= verD2009) caiStop = 0xFF;
    }

    while (1)
    {
        Tag = ReadByte();
        switch (Tag)
        {
        case 1:
            Result = ReadUIndex();
            RefAddrDef(Result);
            F = ReadUIndex();
            if (IsMSIL)
            {
                Len = ReadUIndex();
                for (i = 1; i <= Len; i++)
                {
                    hDef = ReadUIndex();
                    RefAddrDef(hDef);
                    V = ReadUIndex();
                    S = ReadNDXStr();
                    if (S != "")
                    {
                        Len1 = ReadUIndex();
                        for (j = 1; j <= Len1; j++)
                        {
                            hDef1 = ReadUIndex();
                            RefAddrDef(hDef1); //Seems that it's required to reserve addr index
                        }
                    }
                }
            }
            if (FVer >= verD2005 && FVer < verK1)
            {
                InlineMask = 0x80000;
                if (FVer >= verD2009)
                {
                    if (F & 0x800000) IP2 = ReadUIndex();
                    if (F & 0x1) S = ReadNDXStr(); //Deprecated
                    InlineMask = 0x40000;
                }
                else if (FVer >= verD2006)
                {
                    if (F & 0x1000000) IP = ReadUIndex();
                }                                         

                if (F & InlineMask)
                {
                   //Very complex structure - corresponds to the new (Ver>=8) inline directive
                   //Fortunately, we can completely ignore all this info, because it is duplicated
                   //as a regular procedure info even for inlines
                    if (FVer >= verD2006)
                    {
                        ReadUIndex();
                        ReadUIndex();
                    }
                    Len = ReadUIndex();
                    SkipBlock(Len);
                    if (FVer >= verDXE2) ReadUIndex();
                    for (i = 1; i <= 5; i++) ReadUIndex();
                    V = ReadUIndex();
                    RefAddrDef(V);
                    Len = ReadUIndex();
                    if (FVer >= verD2009)
                    {
                        ReadUIndex();
                        ReadUIndex();
                        Len1 = ReadUIndex();
                        SkipBlock(Len1*sizeof(int));
                    }
                    for (i = 1; i <= Len; i++)
                    {
                        V = ReadUIndex();
                        if (FVer >= verD2009)
                        {
                            RefAddrDef(V); //Seems that it's required to reserve addr index (1)
                            //Same as (2), looks like the field was relocated
                            ReadUIndex();
                            ReadUIndex();
                        }
                        V = ReadUIndex();
                        if (FVer < verD2009) RefAddrDef(V); //Seems that it's required to reserve addr index (2)
                        Z = ReadUIndex();
                        if (FVer >= verD2009 && Z)
                            ReadUIndex();
                        if (FVer >= verD2010) ReadUIndex();
                    }
                    Len = ReadUIndex();
                    for (i = 1; i <= Len; i++)
                    {
                        V = ReadUIndex();
                        if (FVer >= verD2009)
                        {
                            switch (V)
                            {
                            case 1:
                                V = ReadUIndex();
                                if (FVer >= verDXE1) RefAddrDef(V); //Perhaps it's required for lower versions too
                                if (FVer >= verDXE1)
                                    V = 2;
                                else
                                    V = 1;
                                break;
                            case 2:
                                V = 1;
                                break;
                            case 3:
                                V = 3;
                                break;
                            case 4:
                                V = 2;
                                break;
                            case 5:
                                V = 4;
                                break;
                            case 6:
                                V = 1;
                                break;
                            default:
                                printf("Error: Unexpected TConstAddInfo.1 LF value: %d\n", V);
                                break;
                            }
                            for (j = 1; j <= V; j++) ReadUIndex();
                        }
                        else
                            V = ReadUIndex();
                    }
                    Len = ReadUIndex(); //Number of units defs from which are used in this def
                    for (i = 1; i <= Len; i++)
                    {
                        hUnit = ReadUIndex();
                        RefAddrDef(hUnit);
                        Len1 = ReadUIndex();
                        for (j = 1; j <= Len1; j++)
                        {
                            V = ReadUIndex();
                            if (hUnit) continue; //Import from another unit - don't care
                            RefAddrDef(V);
                        }
                    }
                    if (FVer >= verD2006)
                    {
                        Len = ReadUIndex();
                        for (i = 1; i <= Len; i++) ReadUIndex();
                        if (FVer >= verD2009)
                        {
                            ReadUIndex();
                            V = ReadUIndex();
                            RefAddrDef(V);
                            ReadUIndex();
                        }
                    }
                }
                if (FVer >= verD2009)
                {
                    if (F & 0x80000) ReadUIndex();
                }
                else
                {
                    if (F & 0x100000) ReadUIndex();
                }
            }
            break;
        case 0x4:
            if (FVer >= verD2006 && FVer < verK1)
            {
                V = ReadUIndex();
                V = ReadUIndex();
            }
            break;
        case 0x6:
            Result = ReadUIndex();
            hDT = ReadUIndex();
            V = ReadUIndex();
            hDef1 = ReadUIndex();
            break;
        case 0x7:
            Result = ReadUIndex();
            hDef1 = ReadUIndex();
            hDef2 = ReadUIndex();
            V = ReadUIndex();
            break;
        case 0x9:
            Result = ReadUIndex();
            hDT = ReadUIndex();
            break;
        case 0xA:
            Result = ReadUIndex();
            V = ReadUIndex();
            F = ReadUIndex();
            hDT = 0;
            if (F & 0x1) hDT = ReadUIndex();
            hDef1 = 0;
            if (F & 0x2) hDef1 = ReadUIndex();
            V2 = 0;
            if (F & 0x4) V2 = ReadUIndex();
            V3 = 0;
            if (F & 0x8) V3 = ReadUIndex();
            V4 = 0;
            if (F & 0x10) V4 = ReadUIndex();
            hDef5 = 0;
            if (F & 0x20) hDef5 = ReadUIndex();
            if (F & 0x40)
            {
                Len = ReadUIndex();
                for (i = 1; i <= Len; i++)
                {
                    S = ReadNDXStr();
                    V = ReadUIndex();
                    V1 = ReadUIndex();
                    hDT = ReadUIndex();
                }
            }
            if (F & 0x80)
            {
                V = ReadUIndex();
                V1 = ReadUIndex();
                V2 = ReadUIndex();
            }
            if (F & 0x100) S = ReadNDXStr();
            if (F & 0x200) S = ReadNDXStr();
            hDef11 = 0;
            if (F & 0x400)
            {
                hDef11 = ReadUIndex();
                if (IsMSIL) RefAddrDef(hDef11);
            }
            hDef12 = 0;
            if (F & 0x800)
            {
                hDef12 = ReadUIndex();
                if (IsMSIL) RefAddrDef(hDef12);
            }
            hDef13 = 0;
            if (F & 0x1000)
            {
                hDef13 = ReadUIndex();
                if (IsMSIL) RefAddrDef(hDef13);
            }
            hDef14 = 0;
            if (F & 0x2000)
            {
                hDef14 = ReadUIndex();
                if (IsMSIL) RefAddrDef(hDef14);
            }
            hDef15 = 0;
            if (F & 0x4000)
            {
                hDef15 = ReadUIndex(); //MSIL 9 only?
                if (IsMSIL) RefAddrDef(hDef15);
            }
            break;
        case 0xC:
            Result = ReadUIndex();
            V1 = ReadUIndex();
            V2 = ReadUIndex();
            break;
        case 0xD:
            if (FVer < verD2005 || FVer >= verK1) return Result;
            //imported unit module information (FileName and version)?
            Result = ReadUIndex();
            S = ReadNDXStr();
            break;
        case 0x10:
            if (FVer < verD2009 || FVer >= verK1) return Result;
            V1 = ReadUIndex();
            RefAddrDef(V1);//???
            V2 = ReadUIndex();
            RefAddrDef(V2);//???
            V3 = ReadUIndex();
            break;
        case 0x12:
            if (FVer < verD2009 || FVer >= verK1) return Result;
            V1 = ReadUIndex();
            RefAddrDef(V1);//???
            V2 = ReadUIndex();
            RefAddrDef(V2);//???
            break;
        case 0x13:
            if (FVer < verD2009 || FVer >= verK1) return Result;
            V1 = ReadUIndex();
            RefAddrDef(V1); //Seems that it's required to reserve addr index
            V2 = ReadUIndex();
            V3 = ReadUIndex();
            S = ReadNDXStr(); //$EXTERNALSYM
            S = ReadNDXStr(); //??
            S = ReadNDXStr(); //$OBJTYPENAME
            break;
        case 0x14:
            if (FVer < verD2009 || FVer >= verK1) return Result;
            V1 = ReadUIndex();
            RefAddrDef(V1);//???
            V2 = ReadUIndex();
            Len = ReadUIndex();
            for (i = 1; i <= Len; i++)
            {
                V1 = ReadUIndex();
                V2 = ReadUIndex();
                V3 = ReadUIndex();
                S = ReadNDXStr();
            }
            break;
        case 0x15:
            if (FVer < verD2009 || FVer >= verK1) return Result;
            V1 = ReadUIndex();
            RefAddrDef(V1);
            V2 = ReadUIndex();   //1
            V3 = ReadUIndex();
            break;
        default:
            if (Tag == caiStop) return Result;
            break;
        }
    }
    return Result;
}
//------------------------------------------------------------------------------
int CurDeclNo = 0;
void __fastcall ReadDeclList(BYTE LK, TNameDecl** Result)
{
int         TTT;
BYTE        B;
int         i, V, X;
BYTE        Tag1;
bool        WasEmbEnd;
bool        brk = false;
TNameDecl   *Embedded;
TNameDecl   **DeclEnd;
TNameDecl   *Decl, *LastProcDecl;

    *Result = 0;
    DeclEnd = Result;
    Embedded = NULL;
    LastProcDecl = NULL;
    FhNextAddr = 0;
    WasEmbEnd = false; //For MSIL only
    while (1)
    {
        Tag1 = FixTag(Tag);
        Decl = NULL;
        switch (Tag1)
        {
        case drType:
            Decl = new TTypeDecl;
            break;
        case drTypeP:
            Decl = new TTypePDecl;
            break;
        case drConst:
            Decl = new TConstDecl;
            break;
        case drResStr:
            Decl = new TResStrDef;
            break;
        case drSysProc:
            if (FVer >= verD8 && FVer < verK1)
                Decl = new TSysProc8Decl;
            else
                Decl = new TSysProcDecl;
         break;
        case drProc:
            Decl = new TProcDecl(Embedded, false);
            LastProcDecl = Decl;
            Embedded = 0;
            break;
        case drEmbeddedProcStart:
            if ((IsMSIL || (FVer >= verD2009 && FVer < verK1)) && WasEmbEnd)
                WasEmbEnd = false;//Just ignore and continue
            else
            {
                FEmbedDepth++;
                if (Embedded)
                {
                    if (!IsMSIL) printf("Warning: Duplicate embedded list\n");
                    Tag = ReadTag();
                    TNameDecl* le = (TNameDecl*)GetDCURecListEnd(Embedded);
                    ReadDeclList(dlEmbedded, &le);
                }
                else
                {
                    Tag = ReadTag();
                    ReadDeclList(dlEmbedded, &Embedded);
                }
                FEmbedDepth--;
                if (Tag != drEmbeddedProcEnd) printf("Warning: Embedded Stop Tag\n");
                if (FVer >= verDXE1 && FVer < verK1) RegisterEmbeddedTypes(&Embedded, FEmbedDepth + 1);//try to fix the local types relocation problem of XE
            }
            break;
        case drVar:
            if (LK == dlArgs || LK == dlArgsT)
                Decl = new TLocalDecl(LK);
            else
                Decl = new TVarDecl;
          break;
        case drThreadVar:
            Decl = new TThreadVarDecl;
            break;
        case drExport:
            Decl = new TExportDecl;
            break;
        case drVarC:
            Decl = new TVarCDecl(false);
            break;
        case arVal:
        case arVar:
        case arResult:
        case arFld:
            Decl = new TLocalDecl(LK);
            break;
        case arAbsLocVar:
            if (LK == dlMain || LK == dlMainImpl)
                Decl = new TAbsVarDecl;
            else
                Decl = new TLocalDecl(LK);
            break;
        case arLabel:
            Decl = new TLabelDecl;
            break;
        case arMethod:
        case arConstr:
        case arDestr:
            Decl = new TMethodDecl(LK);
            break;
        case arClassVar:
            if (!(FVer >= verD2006 && FVer < verK1))
                brk = true;
            else
                Decl = new TClassVarDecl(LK);
            break;
        case arProperty:
            if (LK == dlDispInterface)
                Decl = new TDispPropDecl(LK);
            else
                Decl = new TPropDecl;
            break;
        case arCDecl:
        case arPascal:
        case arStdCall:
        case arSafeCall:
            break;
        case arSetDeft:
            Decl = new TSetDeftInfo;
            break;
        case drStop2:
            if (FVer >= verD8 && FVer < verK1)
                ReadULong();
            break;
        case drStrConstRec:
            if (!(FVer >= verD8 && FVer < verK1))
                brk = true;
            else
                Decl = new TStrConstDecl;
            break;
        case drSpecVar:
            if (!(FVer >= verD2006 && FVer < verK1))
                brk = true;
            else
                Decl = new TSpecVar;
            break;
        case drRangeDef:
        case drChRangeDef:
        case drBoolRangeDef:
        case drWCharRangeDef:
        case drWideRangeDef:
            new TRangeDef;
            break;
        case drEnumDef:
            new TEnumDef;
            break;
        case drFloatDef:
            new TFloatDef;
            break;
        case drPtrDef:
            new TPtrDef;
            break;
        case drTextDef:
            new TTextDef;
            break;
        case drFileDef:
            new TFileDef;
            break;
        case drSetDef:
            new TSetDef;
            break;
        case drShortStrDef:
            new TShortStrDef;
            break;
        case drStringDef:
        case drWideStrDef:
            new TStringDef;
            break;
        case drArrayDef:
            new TArrayDef(false);
            break;
        case drVariantDef:
            new TVariantDef;
            break;
        case drObjVMTDef:
            new TObjVMTDef;
            break;
        case drRecDef:
            new TRecDef;
            break;
        case drProcTypeDef:
            new TProcTypeDef;
            break;
        case drObjDef:
            new TObjDef;
            break;
        case drClassDef:
            new TClassDef;
            break;
        case drMetaClassDef:
            if (!(FVer >= verD8 && FVer < verK1))
                brk = true;
            else
                new TMetaClassDef;
            break;
        case drInterfaceDef:
            new TInterfaceDef;
            break;
        case drVoid:
            new TVoidDef;
            break;
        case drCBlock:
            if (LK != dlMain)
                brk = true;
            else
            {
                if (FDataBlPtr) printf("Warning: 2nd Data block\n");
                FDataBlSize = ReadUIndex();
                FDataBlPtr = ReadMem(FDataBlSize);
            }
            break;
        case drFixUp:
            LoadFixups();
            break;
        case drEmbeddedProcEnd:
            if (!(LK == dlArgsT && FVer > verD3 || LK == dlArgs && FVer > verD3))
                brk = true;
            else
                if (IsMSIL || (FVer >= verD2009 && FVer < verK1)) WasEmbEnd = true;
            break;
        case drCodeLines:
            LoadCodeLines();
            break;
        case drLinNum:
            LoadLineRanges();
            break;
        case drStrucScope:
            LoadStrucScope();
            break;
        case drLocVarTbl:
            LoadLocVarTbl();
            break;
        case drSymbolRef:
            LoadSymbolInfo();
            break;
        case drUnitAddInfo:
            if (!(FVer >= verD7 && FVer < verK1))
                brk = true;
            else
                Decl = new TUnitAddInfo;
            break;
        case drConstAddInfo:
            if (!(FVer >= verD7 && FVer < verK1 || FVer >= verK3))
                brk = true;
            else
                ReadConstAddInfo(LastProcDecl);
            break;
        case drProcAddInfo:
            if (!(FVer >= verD7 && FVer < verK1))
                brk = true;
            else
            {
                V = ReadIndex();
                SetProcAddInfo(V);
            }
            break;
        case drORec:
            if (!(FVer >= verD8 && FVer < verK1))
                brk = true;
            else
            {
                if (FVer >= verD2009)
                    Decl = new TORecDecl;
                else
                    ReadUIndex();
            }
            break;
        case drInfo98:
            if (!(FVer >= verD8 && FVer < verK1))
                brk = true;
            else
            {
                ReadUIndex();
                ReadUIndex();
            }
            break;
        case drCLine://Lines of C text, just ignore them by now
            if (!(FVer >= verD2006 && FVer < verK1))
                brk = true;
            else
            {
                if (FVer >= verDXE1 && FVer<verK1) X = ReadByte();
                V = ReadUIndex();//Length of the line
                SkipBlock(V);//Line chars
            }
            break;
        case drA1Info://Some record of 6 indices, ignore it completely
            if (!(FVer >= verD2006 && FVer < verK1))
                brk = true;
            else
            {
                ReadUIndex();
                ReadUIndex();
                ReadUIndex();
                ReadUIndex();
                V = ReadUIndex();
                for (i = 1; i <= V; i++) ReadUIndex();
            }
            break;
        case drA2Info:
            if (!(FVer >= verD2006 && FVer < verK1))
                brk = true;
            break;
        case arCopyDecl:
            if (!(FVer >= verD2006 && FVer < verK1))
                brk = true;
            else
                Decl = new TCopyDecl;
            break;
        case drA5Info:
            if (!(FVer >= verD2009 && FVer < verK1))
                brk = true;
            //No data for this tag
            break;
        case drA6Info:
            if (!(FVer >= verD2009 && FVer < verK1))
                brk = true;
            else
                new TA6Def;
            break;
        case drA7Info:
            if (!(FVer >= verD2009 && FVer < verK1))
                brk = true;
            else
                new TA7Def;
            break;
        case drA8Info:
            if (!(FVer >= verD2009 && FVer < verK1))
                brk = true;
            else
                ReadUIndex();
            break;
        case drDynArrayDef:
            if (!(FVer >= verD2009 && FVer < verK1))
                brk = true;
            else
                new TDynArrayDef;
            break;
        case drTemplateArgDef:
            if (!(FVer >= verD2009 && FVer < verK1))
                brk = true;
            else
                new TTemplateArgDef;
            break;
        case drTemplateCall:
            if (!(FVer >= verD2009 && FVer < verK1))
                brk = true;
            else
                new TTemplateCall;
            break;
        case drUnicodeStringDef:
            if (!(FVer >= verD2009 && FVer < verK1))
                brk = true;
            else
                new TStringDef;
            break;
        case drDelayedImpInfo:
            if (!(FVer >= verD2009 && FVer < verK1))
                brk = true;
            else
                Decl = new TDelayedImpRec;
            break;
        default:
            brk = true;
            break;
        }
        if (brk) break;
        if (Decl)
        {
            CurDeclNo++;
            *DeclEnd = Decl;
            DeclEnd = &(TNameDecl*)Decl->Next;
        }
        Tag = ReadTag();
    }
    if (Embedded)
    {
        if ((IsMSIL && LK == dlEmbedded) || (FVer >= verD2010 && FVer <= verK1 && (LK == dlArgs || LK == dlArgsT)) || (LK == dlEmbedded && !*Result))
        {
            //A lot of files contain additional drEmbeddedProcStart - drEmbeddedProcEnd
            //brackets for aux record
            *DeclEnd = Embedded;
        }
        else
        {
            FreeDCURecList(Embedded);
            printf("Error: Unused embedded list\n");
        }
    }
}
//------------------------------------------------------------------------------
//TDeclSepFlags
#define dsComma         0
#define dsLast          1
#define dsNoFirst       2
#define dsNL            3
#define dsSoftNL        4
#define dsSmallSameNL   5
#define dsOfsProc       6

String  SecNames[] = {
    "",
    "label",
    "const",
    "type",
    "var",
    "threadvar",
    "resourcestring",
    "exports",
    "",
    "private",
    "protected",
    "public",
    "published"
};

void __fastcall ShowDeclList(BYTE LK, TNameDecl* Decl, String& OutS)
{
    bool        Visible;
    BYTE        SK;
    int         DeclCnt;
    TTypeDef    *TD;
    String      SecN, S;

    OutS = "";
    DeclCnt = 0;
    while (Decl)
    {
        Visible = Decl->IsVisible(LK);
        if (Visible)
        {
            SK = Decl->GetSecKind();
            if (DeclCnt > 0)
            {
                OutS += ";";
                OutLog1(";\n");    //!!! prototype separator
            }
            if (SK >= 9 && SK <= 12)
                ActiveScope = SK;
            else
                ActiveScope = 0;
            if (SK == 2)    //const
            {
                ActiveInfo = 2;
            }
            else if (SK == 3)   //type
            {
                ActiveInfo = 3;
            }
            else if (SK == 4)   //var
            {
                ActiveInfo = 4;
            }
            else if (SK == 5)   //threadvar
            {
                ThreadVar = true;
                ActiveInfo = 5;
            }
            else if (SK == 6)   //resourcestring
            {
                ActiveInfo = 6;
            }
            
            SecN = SecNames[SK];
            if (SecN != "") OutLog2("%s ", SecN.c_str());

            if (LK == dlMain)
                Decl->ShowDef(false, S);
            else if (LK == dlMainImpl)
                Decl->ShowDef(true, S);
            else if (LK == dlA6)
            {
                TD = 0;
                if (Decl->InheritsFrom(__classid(TTypeDecl)))
                {
                    TD = GetTypeDef(((TTypeDecl*)Decl)->hDef);
                    if (TD && TD->InheritsFrom(__classid(TTemplateArgDef)))
                        Decl->ShowName(S);
                    else
                        TD = 0;
                }
                if (!TD) Decl->Show(S); //Just in case
            }
            else
            {
                if (LK == dlClass)  //Class
                {
                    TList *SFieldsList = FieldsList;
                    TList *SPropertiesList = PropertiesList;
                    TList *SMethodsList = MethodsList;
                    TList *SArgsList = ArgsList;
                    FieldsList = 0;
                    PropertiesList = 0;
                    MethodsList = 0;
                    ArgsList = 0;

                    if (Decl->ClassNameIs("TLocalDecl"))
                        FieldsList = SFieldsList;
                    else if (Decl->ClassNameIs("TPropDecl"))
                        PropertiesList = SPropertiesList;
                    else if (Decl->ClassNameIs("TMethodDecl"))
                        MethodsList = SMethodsList;

                    Decl->Show(S);

                    FieldsList = SFieldsList;;
                    PropertiesList = SPropertiesList;
                    MethodsList = SMethodsList;
                    ArgsList = SArgsList;
                }
                else if (LK == dlArgsT) //Interface
                {
                    TList *SFieldsList = FieldsList;
                    TList *SMethodsList = MethodsList;
                    TList *SArgsList = ArgsList;
                    TList *SLocalsList = LocalsList;
                    FieldsList = 0;
                    MethodsList = 0;
                    ArgsList = 0;
                    LocalsList = 0;

                    Decl->Show(S);

                    FieldsList = SFieldsList;;
                    MethodsList = SMethodsList;
                    ArgsList = SArgsList;
                    LocalsList = SLocalsList;
                }
                else if (LK == dlFields)
                {
                    Decl->Show(S);
                }
                else if (LK == dlEmbedded)
                {
                    TList *SConstList = ConstList;
                    TList *STypeList = TypeList;
                    ConstList = 0;
                    TypeList = 0;
                    Decl->Show(S);
                    ConstList = SConstList;
                    TypeList = STypeList;
                }
                else
                    Decl->Show(S);
            }
            DeclCnt++;
            OutS += S;
        }
        Decl = (TNameDecl*)Decl->Next;
    }
}
//------------------------------------------------------------------------------
String __fastcall ShowRefOfsQualifier(int hDef, int Ofs)
{
    TTypeDef    *TD;

    TD = GetGlobalTypeDef(hDef);
    if (!TD)
    {
        if (Ofs > 0) return Format("^+%d", ARRAYOFCONST((Ofs)));
        if (Ofs < 0) return Format('^%d', ARRAYOFCONST((Ofs)));
        return "";
    }
    return TD->GetRefOfsQualifier(Ofs);
}
//------------------------------------------------------------------------------
String __fastcall ShowTypeDef(int hDef, PName N)
{
    TBaseDef    *D;
    String      Result;

    Result = "";
    D = GetTypeDef(hDef);
    if (D)
        D->ShowNamed(N, Result);
    else
    {
        Result = "?";
        OutLog1("?");
    }
    return Result;
}
//------------------------------------------------------------------------------
String __fastcall ShowTypeName(int hDef)
{
    TBaseDef *D;
    PName N;
    String S = "";

    if (hDef <= 0 || hDef > FTypes->Count) return S;
    D = (TBaseDef*)FTypes->Items[hDef-1];
    if (!D) return S;
    N = D->FName;
    if (!N || !N->Len) return S;
    D->ShowName(S);
    return S;
}
//------------------------------------------------------------------------------
//Version   fxStart fxEnd   fxAdr   fxJmp   fxS fxData
//verD7     6       7       1       2       5   3
//verD2006  C       D       1       2       5   3
//verD2009  C       D       1       2       5   3
//verD2010  0       1       4       18      5   8
void __fastcall ShowDump(
    BYTE* DP,                       //File 0 address, show file offsets if present
    BYTE* DPFile0,                  //Dump address
    DWORD FileSize, DWORD SizeDispl,//used to calculate display offset digits
    DWORD Size,                     //Dump size
    DWORD Ofs0Displ,                //initial display offset
    DWORD Ofs0,                     //offset in DCU data block - for fixups
    DWORD WMin,                     //Minimal dump width (in bytes)
    int FixCnt, TFixupRec* FixTbl)
{
    BYTE *LP;
    DWORD W;
    DWORD LSz, dOfs, ROfs;
    PFixupRec FP;
    BYTE K, B;

    if ((int)Size <= 0) return;

    if (DPFile0)
        ROfs = DP - DPFile0;
    else
        ROfs = DP - FMemPtr;

    LP = DP;

    if (pDumpOffset)
        *pDumpOffset = ROfs;
    if (pDumpSize)
        *pDumpSize = Size;

    OutLog3("{Ofs:%lX Sz:%lX}\n", ROfs, Size);
    for (int n = 0; n < Size; n++)
    {
        if (n) OutLog1(" ");
        OutLog2("%02X", *(LP + n));
    }

    OutLog1("\n");

    W = 16;
    if (Size < W)
    {
        W = Size;
        if (W < WMin) W = WMin;
    }
    FP = FixTbl;
    if (!FP) FixCnt = 0;

    String Name;
    PFIXUPINFO finfo;

    do
    {
        LSz = W;
        if (LSz > Size) LSz = Size;
        while (FixCnt > 0)
        {
            dOfs = (FP->OfsF & FixOfsMask) - Ofs0;
            K = ((BYTE*)&FP->OfsF)[3];
            if (dOfs >= LSz && (dOfs != LSz || K != fxEnd)) break;
            if (FVer == verD2010 || FVer == verDXE1 || FVer == verDXE2)
            {
                if (K == 4 || K == 5 || K == 6 || K == 8 || K == 18)
                {
                    finfo = new FIXUPINFO;
                    finfo->Ofs = LP - DP + dOfs;
                    OutLog2("%lX: ", LP - DPFile0 + dOfs);
                    if (K == 4)
                    {
                        finfo->Type = 'A';
                        OutLog1("A");
                    }
                    else if (K == 5)
                    {
                        finfo->Type = 'J';
                        OutLog1("J");
                    }
                    else if (K == 6)
                    {
                        finfo->Type = 'D';
                        OutLog1("D");
                    }
                    else if (K == 8)
                    {
                        finfo->Type = 'S';
                        OutLog1("S");
                    }
                    else if (K == 18)   //???
                    {
                        finfo->Type = 'B';
                        OutLog1("B");
                    }
                    else
                    {
                        finfo->Type = 'U';
                        OutLog1("U");
                    }
                    Name = GetAddrStr(FP->Ndx);
                    finfo->Name = Name;
                    if (FixupsList) FixupsList->Add((void*)finfo);
                    OutLog2(" %s\n", Name.c_str());
                }
            }
            else if (FVer == verD7 || FVer == verD2006 || FVer == verD2009)
            {
                if (K == 1 || K == 2 || K == 3 || K == 5)
                {
                    finfo = new FIXUPINFO;
                    finfo->Ofs = LP - DP + dOfs;
                    OutLog2("%lX: ", LP - DPFile0 + dOfs);
                    if (K == 1)
                    {
                        finfo->Type = 'A';
                        OutLog1("A");
                    }
                    else if (K == 2)
                    {
                        finfo->Type = 'J';
                        OutLog1("J");
                    }
                    else if (K == 3)
                    {
                        finfo->Type = 'D';
                        OutLog1("D");
                    }
                    else if (K == 5)
                    {
                        finfo->Type = 'S';
                        OutLog1("S");
                    }
                    else
                    {
                        finfo->Type = 'U';
                        OutLog1("U");
                    }
                    Name = GetAddrStr(FP->Ndx);
                    finfo->Name = Name;
                    if (FixupsList) FixupsList->Add((void*)finfo);
                    OutLog2(" %s\n", Name.c_str());
                }
            }
            else
            {
                if (K == 1 || K == 2 || (K == 3 && FVer != verD2))
                {
                    finfo = new FIXUPINFO;
                    finfo->Ofs = LP - DP + dOfs;
                    OutLog2("%lX: ", LP - DPFile0 + dOfs);
                    if (K == 1)
                    {
                        finfo->Type = 'A';
                        OutLog1("A");
                    }
                    else if (K == 2)
                    {
                        finfo->Type = 'J';
                        OutLog1("J");
                    }
                    else if (K == 3)
                    {
                        finfo->Type = 'D';
                        OutLog1("D");
                    }
                    else if (K == 5)
                    {
                        finfo->Type = 'S';
                        OutLog1("S");
                    }
                    else
                    {
                        finfo->Type = 'U';
                        OutLog1("U");
                    }
                    Name = GetAddrStr(FP->Ndx);
                    finfo->Name = Name;
                    if (FixupsList) FixupsList->Add((void*)finfo);
                    OutLog2(" %s\n", Name.c_str());
                }
            }
            FixCnt--;
            FP++;
        }

        Ofs0 += LSz;
        Size -= LSz;
        LP += LSz;
    } while (Size > 0);
}
//------------------------------------------------------------------------------
BYTE* __fastcall GetBlockMem(DWORD BlOfs, DWORD BlSz, DWORD* ResSz)
{
    *ResSz = BlSz;
    if (!FDataBlPtr || (int)BlOfs < 0 || !BlSz) return NULL;
    if (BlSz + BlOfs > FDataBlSize)
    {
        BlSz = FDataBlSize - BlOfs;
        if ((int)BlSz <= 0) return NULL;
    }
    *ResSz = BlSz;
    return FDataBlPtr + BlOfs;
}
//------------------------------------------------------------------------------
int __fastcall GetStartFixup(DWORD Ofs)
{
    int i, iMin, iMax;
    int d;

    if (!FFixupTbl || !FFixupCnt) return 0;
    if (!Ofs) return 0;
    iMin = 0;
    iMax = FFixupCnt - 1;
    while (iMin <= iMax)
    {
        i = (iMin + iMax)/2;
        TFixupRec fRec = FFixupTbl[i];
        d = (fRec.OfsF & FixOfsMask) - Ofs;
        if (d < 0)
            iMin = i + 1;
        else
            iMax = i - 1;
    }
    return iMin;
}
//------------------------------------------------------------------------------
void __fastcall ShowDataBl(DWORD Ofs0, DWORD BlOfs, DWORD BlSz)
{
    int Fix0;
    BYTE *DP;

    DP = GetBlockMem(BlOfs + Ofs0, BlSz - Ofs0, &BlSz);
    if (!DP) return;
    Fix0 = GetStartFixup(BlOfs + Ofs0);
    ShowDump(DP, FMemPtr, FMemSize, 0, BlSz, Ofs0, BlOfs + Ofs0, 0, FFixupCnt - Fix0, &FFixupTbl[Fix0]);
}
//------------------------------------------------------------------------------
void __fastcall DasmCodeBlSeq(DWORD Ofs0, DWORD BlOfs, DWORD BlSz)
{
    BYTE *DP;
    int Fix0;
    char *FOfs0;

    DP = GetBlockMem(BlOfs, BlSz, &BlSz);
    if (!DP)
    {
        printf("Warning: nodump\n");
        return;
    }
    Fix0 = GetStartFixup(BlOfs);
    FOfs0 = FMemPtr;
    ShowDump(DP, FOfs0, FMemSize, BlSz, BlSz, Ofs0, BlOfs, 0, FFixupCnt-Fix0, &FFixupTbl[Fix0]);
}
//------------------------------------------------------------------------------
void __fastcall ShowCodeBl(DWORD Ofs0, DWORD BlOfs, DWORD BlSz)
{
    DWORD Sz, CodeSz;

    CodeSz = BlSz;
    DasmCodeBlSeq(Ofs0, BlOfs, CodeSz);
    if (CodeSz < BlSz)
    {
        OutLog1("rest:\n");
        ShowDataBl(Ofs0 + CodeSz, BlOfs + CodeSz, Ofs0 + BlSz);
    }
}
//------------------------------------------------------------------------------
bool __fastcall MemToUInt(BYTE *DP, DWORD Sz, DWORD* Res)
{
    switch (Sz)
    {
    case 1:
        *Res = *((BYTE*)DP);
        break;
    case 2:
        *Res = *((WORD*)DP);
        break;
    case 4:
        *Res = *((DWORD*)DP);
        break;
    default:
        Res = 0;
        return false;
    }
    return true;
}
//------------------------------------------------------------------------------
String __fastcall CharStr(char Ch)
{
    char buf[256];
    if (Ch < ' ')
        sprintf(buf, "#%d", (BYTE)Ch);
    else
        sprintf(buf, "'%c'", Ch);
    return String(buf);
}
//------------------------------------------------------------------------------
String __fastcall StrConstStr(char* CP, int L)
{
    bool WasCode, Code;
    char Ch;
    int LRes;

    String Result;
    Result.SetLength(3*L + 2);
    LRes = 0;
    Code = true;
    while (L > 0)
    {
        Ch = *CP;
        CP++; L--;
        WasCode = Code;
        Code = (Ch < ' ');
        if (WasCode != Code)
        {
            LRes++;
            Result[LRes] = '\'';
        }
        if (Code)
        {
            String S = CharStr(Ch);
            Move(&S[1], &Result[LRes + 1], S.Length());
            LRes += S.Length();
        }
        else
        {
            if (Ch == '\'')
            {
                LRes++;
                Result[LRes] = '\'';
            }
            LRes++;
            Result[LRes] = Ch;
        }
    }
    if (!Code)
    {
        LRes++;
        Result[LRes] = '\'';
    }
    if (!LRes)
        Result = "''";
    else
        Result.SetLength(LRes);
    return Result;
}
//------------------------------------------------------------------------------
int __fastcall ShowStrConst(BYTE* DP, DWORD DS, String& OutS)
{
    int L;
    BYTE *VP;
    String Value;
    OutS = "";

    if (DS < 9) return -1;
    if ((*(int*)DP) != -1) return -1;
    VP = DP + sizeof(int);
    L = (*(int*)VP);
    if (DS < L + 9) return -1;
    VP += sizeof(int);
    if (*(VP + L)) return -1;
    Value = StrConstStr(VP, L);
    OutS = Value;
    OutLog2("%s", Value.c_str());
    return L + 9;
}
//------------------------------------------------------------------------------
//The unicode string support for ver>=verD12
//New string header:
//  -12:2 - Code page
//  -10:2 - string item size
//  -8:4 - reference count
//  -4:4 - Length in terms of the string item size
int __fastcall ShowUnicodeStrConst(BYTE* DP, DWORD DS, String& OutS) //Ver >=verD12
{
    int     ElSz, Result = -1;
    BYTE    *VP;

    OutS = "";
    if (DS < 13) return Result;
    VP = DP + sizeof(WORD);
    ElSz = *((WORD*)VP);
    if (ElSz == sizeof(AnsiChar)) //Try to show it as an AnsiString (!!!the code page is ignored by now)
    {
        Result = ShowStrConst(DP + 2*sizeof(WORD), DS - 2*sizeof(WORD), OutS);
        if (Result > 0) Result += 2*sizeof(WORD);
        return Result;
    }
    if (ElSz != sizeof(WideChar)) return Result;
    VP = DP + sizeof(int);
    if (*((int*)VP) != -1) return Result;
    VP += sizeof(int);
    Result = ShowUnicodeResStrConst(VP, DS - 2*sizeof(int), OutS);
    if (Result < 0) return Result;
    Result += 2*sizeof(int);
    return Result;
}
//------------------------------------------------------------------------------
int __fastcall ShowUnicodeResStrConst(BYTE* DP, DWORD DS, String& OutS) //Ver >=verD12
{
    int         L, Result = -1;
    WideString  WS;
    String      S;

    OutS = "";
    L = *((int*)DP);
    if (DS - 6 < L*sizeof(WideChar) || L < 0) return Result;
    DP += sizeof(int);
    if (*(PWideChar)(DP + L*sizeof(WideChar)) != 0) return Result;
    WS = WideString((PWideChar)DP, L);
    S = WS;
    Result = L*sizeof(WideChar) + 6;
    OutS = StrConstStr(S.c_str(), L);
    OutLog2("%s", OutS.c_str());
}
//------------------------------------------------------------------------------
int __fastcall ShowTypeValue(TTypeDef* T, BYTE* DP, DWORD DS, int ConstKind, String& OutS)
{
    int Result;
    TFixupMemState MS;

    OutS = "";
    if (!T) return -1;
    if (ConstKind > 0)
    {
        SaveFixupMemState(&MS);
        SetCodeRange(DP, DP, DS);
    }
    if (ConstKind > 0 && T->InheritsFrom(__classid(TStringDef)))
    {
        if (FVer >= verD2009 && FVer < verK1)
        {
            if (ConstKind == 2)
                Result = ShowUnicodeResStrConst(DP, DS, OutS);
            else
                Result = ShowUnicodeStrConst(DP, DS, OutS);
        }
        else
            Result = ShowStrConst(DP, DS, OutS);
    }
    else
        Result = T->ShowValue(DP, DS, OutS);
    if (ConstKind > 0)
        RestoreFixupMemState(&MS);
    return Result;
}
//------------------------------------------------------------------------------
int __fastcall ShowGlobalTypeValue(int hDef, BYTE* DP, DWORD DS, bool AndRest, int ConstKind, String& OutS)
{
    TTypeDef    *T;
    int         SzShown;
    BYTE        *FOfs0;

    OutS = "";
    if (!DP) return -1;
    T = GetGlobalTypeDef(hDef);
    int Result = ShowTypeValue(T, DP, DS, ConstKind, OutS);
    if (!AndRest) return Result;
    SzShown = Result;
    if (SzShown < 0) SzShown = 0;
    if (SzShown >= DS) return Result;
    if (DP >= FDataBlPtr && DP < FDataBlPtr + FDataBlSize)
        ShowDataBl(SzShown, DP - FDataBlPtr, DS);
    else
    {
        FOfs0 = FMemPtr;
        ShowDump(DP, FOfs0, FMemSize, 0, DS, SzShown, SzShown, 0, 0, NULL);
    }
    return Result;
}
//------------------------------------------------------------------------------
TDCURec* __fastcall GetGlobalAddrDef(int hDef)
{
    TDCURec *D;

    D = GetAddrDef(hDef);
    if (!D) return NULL;
    if (D->InheritsFrom(__classid(TImpDef))) return NULL;
    return D;
}
//------------------------------------------------------------------------------
bool __fastcall ShowGlobalConstValue(int hDef, String& OutS)
{
    TDCURec *D;

    D = GetGlobalAddrDef(hDef);
    if (!D || !D->InheritsFrom(__classid(TConstDecl))) return false;
    ((TConstDecl*)D)->ShowValue(OutS);
    return true;
}
//------------------------------------------------------------------------------
String __fastcall ShowOfsQualifier(int hDef, int Ofs)
{
    TTypeDef    *TD;

    TD = GetGlobalTypeDef(hDef);
    if (!TD)
    {
        if (Ofs > 0)
            return Format("+%d", ARRAYOFCONST((Ofs)));
        else if (Ofs < 0)
            return Format("%d", ARRAYOFCONST((Ofs)));
        return "";
    }
    return TD->GetOfsQualifier(Ofs);
}
//------------------------------------------------------------------------------
bool __fastcall ScanOneDCU(String Filename)
{
BYTE        B;
String      S, SName;
DWORD       L, L1, L2, Flags1;

    FILE *fIn = fopen(Filename.c_str(), "rb");
    if (!fIn) return false;

    ModuleInfo = new MODULEINFO;
    ModuleInfo->ModuleID = ModuleList->Count;
    ModuleInfo->UsesList = new TStringList;
    ModuleList->Add((void*)ModuleInfo);
    ModuleID = ModuleInfo->ModuleID;
    ModuleInfo->Filename = Filename;

    FUnitImp->Clear();
    FTypes->Clear();
    FAddrs->Clear();
    FTypeShowStack->Clear();

    GenVarCAsVars = false;
    FVer = 0;
    fxStart = fxStart30;
    fxEnd = fxEnd30;
    DefStart = 0;
    FExportNames = 0;
    FDecls = 0;
    FTypeDefCnt = 0;
    FDataBlPtr = 0;
    FFixupTbl = 0;
    FLocVarTbl = 0;

    fseek(fIn, 0, SEEK_END);
    FMemSize = ftell(fIn);
    fseek(fIn, 0, SEEK_SET);
    FMemPtr = new BYTE[FMemSize];
    fread((void*)FMemPtr, 1, FMemSize, fIn);
    fclose(fIn);

    CurPos = FMemPtr;
    IsMSIL = false;
    //Read Magic
    Magic = ReadULong();
    FPtrSize = 4;
    fxJmpAddr = fxJmpAddr0;
    
    switch (Magic)
    {
    case 0x50505348:
        FVer = verD2;
        fxStart = fxStart20;
        fxEnd = fxEnd20;
        break;
    case 0x44518641:
        FVer = verD3;
        fxStart = fxStart30;
        fxEnd = fxEnd30;
        break;
    case 0x4768A6D8:
        FVer = verD4;
        fxStart = fxStart30;
        fxEnd = fxEnd30;
        break;
    case 0xF21F148B:
        FVer = verD5;
        fxStart = fxStart30;
        fxEnd = fxEnd30;
        break;
    case 0x0E0000DD:
    case 0x0E8000DD:
        FVer = verD6;
        fxStart = fxStart30;
        fxEnd = fxEnd30;
        break;
    case 0xFF0000DF:
    case 0x0F0000DF:
    case 0x0F8000DF:
        FVer = verD7;
        fxStart = fxStart70;
        fxEnd = fxEnd70;
        break;
    case 0x10000229:
        FVer = verD8;
        IsMSIL = true;
        fxStart = fxStartMSIL;
        fxEnd = fxEndMSIL;
        break;
    case 0x11000239:
        FVer = verD2005;
        IsMSIL = true;
        fxStart = fxStartMSIL;
        fxEnd = fxEndMSIL;
        break;
    case 0x1100000D:
    case 0x11800009:
        FVer = verD2005;
        fxStart = fxStart70;
        fxEnd = fxEnd70;
        break;
    case 0x12000023:
        FVer = verD2006;	    //Delphi 2006, 2007
        fxStart = fxStart100;
        fxEnd = fxEnd100;
        break;
    case 0x1200024D:
        FVer = verD2006;
        IsMSIL = true;
        fxStart = fxStartMSIL;
        fxEnd = fxEndMSIL;
        break;
    case 0x14000039:
        FVer = verD2009;    //Delphi 2009
        fxStart = fxStart100;
        fxEnd = fxEnd100;
        break;
    case 0x15000045:
    	FVer = verD2010;    //Delphi 2010
        fxStart = fxStart2010;
        fxEnd = fxEnd2010;
        fxJmpAddr = fxJmpAddrXE; //Was checked for XE only
        break;
    case 0x1600034B:
        FVer = verDXE1;    //DelphiXE1
        fxStart = fxStart2010;
        fxEnd = fxEnd2010;
        fxJmpAddr = fxJmpAddrXE; //Was checked for XE only
        break;
    case 0x1700034B:
        FVer = verDXE2;    //DelphiXE2
        fxStart = fxStart2010;
        fxEnd = fxEnd2010;
        fxJmpAddr = fxJmpAddrXE; //Was checked for XE only
        break;
    case 0x1700234B:
        FVer = verDXE2;    //DelphiXE2
        FPlatform = dcuplWin64;
        FPtrSize = 8;
        fxJmpAddr = fxJmpAddrXE; //Was checked for XE only
        break;
    case 0x1700044B:
        FVer = verDXE2;    //DelphiXE2
        FPlatform = dcuplOsx32;
        fxJmpAddr = fxJmpAddrXE; //Was checked for XE only
        break;
    case 0x1800034B:
        FVer = verDXE3;    //DelphiXE3
        fxStart = fxStart2010;
        fxEnd = fxEnd2010;
        fxJmpAddr = fxJmpAddrXE; //Was checked for XE only
        break;
    case 0x1800234B:
        FVer = verDXE3;    //DelphiXE3
        FPlatform = dcuplWin64;
        FPtrSize = 8;
        fxJmpAddr = fxJmpAddrXE; //Was checked for XE only
        break;
    case 0x1800044B:
        FVer = verDXE3;    //DelphiXE3
        FPlatform = dcuplOsx32;
        fxJmpAddr = fxJmpAddrXE; //Was checked for XE only
        break;
    case 0xF21F148C:
        FVer = verK1; 	    //Kylix 1.0
        fxStart = fxStart30;
        fxEnd = fxEnd30;
        break;
    case 0x0E1011DD:
    case 0x0E0001DD:
        FVer = verK2; 	    //Kylix 2.0
        fxStart = fxStart30;
        fxEnd = fxEnd30;
        break;
    case 0x0F1001DD:
    case 0x0F0001DD:
        FVer = verK3; 	    //Kylix 3.0
        break;
    default:
        printf("Error: Wrong magic %lX\n", Magic);
        delete[] FMemPtr;
        return false;
    }
    /*
    if (fxStart > 0)
      fxValid = [0..fxStart-1];
    else if (FVer < verDXE2 || FPlatform <> dcuplWin64)
      fxValid = [fxEnd+1..fxMaxXE];
    else
      fxValid = [fxEnd+1..fxMax];
    */
    //Read File Header
    FileSizeH = ReadULong();
    if (FileSizeH != FMemSize)
    {
        printf("Error: Wrong size: %lX != %lX\n", FileSizeH, FMemSize);
        delete[] FMemPtr;
        return false;
    }

    //Read FileTime
    FT = ReadULong();
    if (FVer == verD2)
    {
        B = ReadByte();
        Tag = ReadTag();
    }
    else
    {
        Stamp = ReadULong();
        B = ReadByte();
        if (FVer >= verD7 && FVer < verK1)
        {
            B = ReadByte(); //It has another header byte (or index)
            AddAddrDef(NULL);
        }
        if (FVer >= verD2005 && FVer < verK1)
        {
            SName = ReadStr();
        }
        if (FVer >= verD2009 && FVer < verK1)
        {
            L1 = ReadUIndex();
            L2 = ReadUIndex();
        }
        Tag = ReadTag();
        if (FVer >= verK1)
        {
            if (Tag == drUnit4)
            {
                do
                {
                    L = ReadULong();
                    Tag = ReadTag();
                } while (Tag != drUnit4);
            }
            else if (Tag != drUnitFlags)
            {
                SkipBlock(3);
                Tag = ReadTag();
            }
        }
        if (Tag == drUnitFlags)
        {
            Flags = ReadUIndex();
            if (FVer > verD2005 && FVer < verK1)
                Flags1 = ReadUIndex();
            if (FVer > verD3)
                UnitPrior = ReadUIndex();
            Tag = ReadTag();
        }
    }
    ReadSourceFiles();
    ReadUses(drUnit);
    ReadUses(drUnit1);
    ReadUses(drDLL);

    ReadDeclList(dlMain, &FDecls);
    if (FVer >= verDXE1 && FVer < verK1)
        BindEmbeddedTypes(); //try to fix the local types relocation problem of XE
    SetExportNames(FDecls);
    SetEnumConsts(&FDecls);
    FillProcLocVarTbls();

    ShowSourceFiles();
    //interface
    ShowUses("uses", 0);
    ShowDeclList(dlMain, FDecls, S);
    //implementation
    ShowUses("uses", ufImpl);
    ShowUses("imports", ufDLL);
    ModuleInfo->UsesList->Sort();
    ShowDeclList(dlMainImpl, FDecls, S);

    FreeDCURecList((TDCURec*)FDecls);
    delete[] FMemPtr;
    return true;
}
//------------------------------------------------------------------------------
//Сортировка по именам
int __fastcall CompareModulesByName(void *Item1, void *Item2)
{
    PMODULEINFO info1 = (PMODULEINFO)Item1;
    PMODULEINFO info2 = (PMODULEINFO)Item2;
    return CompareText(info1->Name, info2->Name);
}
//------------------------------------------------------------------------------
//Сортировка по ModuleID
int __fastcall CompareModulesByID(void *Item1, void *Item2)
{
    PMODULEINFO info1 = (PMODULEINFO)Item1;
    PMODULEINFO info2 = (PMODULEINFO)Item2;
    if (info1->ModuleID > info2->ModuleID) return 1;
    if (info1->ModuleID < info2->ModuleID) return -1;
    return 0;
}
//------------------------------------------------------------------------------
int __fastcall CompareConstsByName(void *Item1, void *Item2)
{
    PCONSTINFO info1 = (PCONSTINFO)Item1;
    PCONSTINFO info2 = (PCONSTINFO)Item2;
    int res = CompareText(info1->Name, info2->Name);
    if (res) return res;
    if (info1->ModuleID > info2->ModuleID) return 1;
    if (info1->ModuleID < info2->ModuleID) return -1;
    return 0;
}
//------------------------------------------------------------------------------
int __fastcall CompareConstsByID(void *Item1, void *Item2)
{
    PCONSTINFO info1 = (PCONSTINFO)Item1;
    PCONSTINFO info2 = (PCONSTINFO)Item2;
    if (info1->ModuleID > info2->ModuleID) return 1;
    if (info1->ModuleID < info2->ModuleID) return -1;
    return CompareText(info1->Name, info2->Name);
}
//------------------------------------------------------------------------------
int __fastcall CompareTypesByName(void *Item1, void *Item2)
{
    PTYPEINFO info1 = (PTYPEINFO)Item1;
    PTYPEINFO info2 = (PTYPEINFO)Item2;
    int res = CompareText(info1->Name, info2->Name);
    if (res) return res;
    if (info1->ModuleID > info2->ModuleID) return 1;
    if (info1->ModuleID < info2->ModuleID) return -1;
    return 0;
}
//------------------------------------------------------------------------------
int __fastcall CompareTypesByID(void *Item1, void *Item2)
{
    PTYPEINFO info1 = (PTYPEINFO)Item1;
    PTYPEINFO info2 = (PTYPEINFO)Item2;
    if (info1->ModuleID > info2->ModuleID) return 1;
    if (info1->ModuleID < info2->ModuleID) return -1;
    return CompareText(info1->Name, info2->Name);
}
//------------------------------------------------------------------------------
int __fastcall CompareVarsByName(void *Item1, void *Item2)
{
    PVARINFO info1 = (PVARINFO)Item1;
    PVARINFO info2 = (PVARINFO)Item2;
    int res = CompareText(info1->Name, info2->Name);
    if (res) return res;
    if (info1->ModuleID > info2->ModuleID) return 1;
    if (info1->ModuleID < info2->ModuleID) return -1;
    return 0;
}
//------------------------------------------------------------------------------
int __fastcall CompareVarsByID(void *Item1, void *Item2)
{
    PVARINFO info1 = (PVARINFO)Item1;
    PVARINFO info2 = (PVARINFO)Item2;
    if (info1->ModuleID > info2->ModuleID) return 1;
    if (info1->ModuleID < info2->ModuleID) return -1;
    return CompareText(info1->Name, info2->Name);
}
//------------------------------------------------------------------------------
int __fastcall CompareResStrsByName(void *Item1, void *Item2)
{
    PRESSTRINFO info1 = (PRESSTRINFO)Item1;
    PRESSTRINFO info2 = (PRESSTRINFO)Item2;
    int res = CompareText(info1->Name, info2->Name);
    if (res) return res;
    if (info1->ModuleID > info2->ModuleID) return 1;
    if (info1->ModuleID < info2->ModuleID) return -1;
    return 0;
}
//------------------------------------------------------------------------------
int __fastcall CompareResStrsByID(void *Item1, void *Item2)
{
    PRESSTRINFO info1 = (PRESSTRINFO)Item1;
    PRESSTRINFO info2 = (PRESSTRINFO)Item2;
    if (info1->ModuleID > info2->ModuleID) return 1;
    if (info1->ModuleID < info2->ModuleID) return -1;
    return CompareText(info1->Name, info2->Name);
}
//------------------------------------------------------------------------------
int __fastcall CompareProcsByName(void *Item1, void *Item2)
{
    PPROCDECLINFO info1 = (PPROCDECLINFO)Item1;
    PPROCDECLINFO info2 = (PPROCDECLINFO)Item2;
    int res = CompareText(info1->Name, info2->Name);
    if (res) return res;
    if (info1->ModuleID > info2->ModuleID) return 1;
    if (info1->ModuleID < info2->ModuleID) return -1;
    return 0;
}
//------------------------------------------------------------------------------
int __fastcall CompareProcsByID(void *Item1, void *Item2)
{
    PPROCDECLINFO info1 = (PPROCDECLINFO)Item1;
    PPROCDECLINFO info2 = (PPROCDECLINFO)Item2;
    if (info1->ModuleID > info2->ModuleID) return 1;
    if (info1->ModuleID < info2->ModuleID) return -1;
    if (info1->ID > info2->ID) return 1;
    if (info1->ID < info2->ID) return -1;
    return 0;
}
//------------------------------------------------------------------------------
#include <dir.h>
//Записываем длину имени (WORD) + имя + нулевой байт, возвращаем общее кол-во байт
int __fastcall WriteString(FILE* fDst, String& str)
{
    int Bytes = 0;
    BYTE ZeroB = 0;

    WORD NameLength = (WORD)str.Length();
    if (fDst) fwrite(&NameLength, sizeof(NameLength), 1, fDst);
    if (NameLength)
    {
        if (fDst) fwrite(str.c_str(), 1, NameLength, fDst);
    }
    if (fDst) fwrite(&ZeroB, 1, 1, fDst);
    return sizeof(NameLength) + NameLength + 1;
}
//------------------------------------------------------------------------------
int __fastcall WriteDump(FILE* fSrc, FILE *fDst, long SrcOffset, DWORD Bytes)
{
    fseek(fSrc, SrcOffset, SEEK_SET);
    for (int m = 0; m < Bytes; m++)
    {
        BYTE b;
        fread(&b, 1, 1, fSrc);
        fwrite(&b, 1, 1, fDst);
    }
    return Bytes;
}
//------------------------------------------------------------------------------
int __fastcall WriteRelocs(FILE *fDst, TList *FixupList, DWORD Bytes, String Name)
{
    BYTE    Byte00 = 0;
    DWORD   ByteFF = 0xFFFFFFFF;
    int     ByteNo = 0;
    for (int n = 0; n < FixupList->Count; n++)
    {
        PFIXUPINFO finfo = (PFIXUPINFO)FixupList->Items[n];
        //Если информация о фиксапе занимает больше места, чем нужно, не записываем ее
        if (finfo->Ofs + 4 > Bytes) continue;
        if (finfo->Ofs < ByteNo) continue;
        //Свободное от релоков место заполняем 0
        for (ByteNo; ByteNo < finfo->Ofs; ByteNo++)
            fwrite(&Byte00, 1, 1, fDst);
        //Сам релок заполняем 4-мя байтами 0xFF
        fwrite(&ByteFF, 1, 4, fDst); ByteNo += 4;
    }
    //Оставшиеся байты заполняем 0
    for (ByteNo; ByteNo < Bytes; ByteNo++)
        fwrite(&Byte00, 1, 1, fDst);

    return Bytes;
}
//------------------------------------------------------------------------------
int __fastcall WriteFixups(FILE *fDst, TList *FixupList)
{
    int Bytes = 0;
    for (int m = 0; m < FixupList->Count; m++)
    {
        PFIXUPINFO finfo = (PFIXUPINFO)FixupList->Items[m];
        //write Type
        if (fDst) fwrite(&finfo->Type, sizeof(finfo->Type), 1, fDst); Bytes += sizeof(finfo->Type);
        //write Offset
        if (fDst) fwrite(&finfo->Ofs, sizeof(finfo->Ofs), 1, fDst); Bytes += sizeof(finfo->Ofs);
        //write Name
        Bytes += WriteString(fDst, finfo->Name);
    }
    return Bytes;
}
//------------------------------------------------------------------------------
#pragma argsused
int main(int argc, char* argv[])
{
    int         num = 0;
    FILE*       fList = 0;
    TSearchRec  sr;
    int         iAttributes = faReadOnly | faArchive;
    char        dcuFilename[256];

    printf("Knowledge Base Builder for IDR by crypto and Alexei Hmelnov\n");

    if (argc != 2 && argc != 3)
    {
        printf("Usage BuildKB KBName <DCUList>\n");
        return -1;
    }

    fOut = fopen(argv[1], "wb+");
    if (!fOut)
    {
        printf("Error: Cannot open KB file %s\n", argv[1]);
        return -1;
    }
    fLog = fopen("BuildKB.Log", "wt+");

    if (argc == 3)
    {
        fList = fopen(argv[2], "rt");
        if (!fList)
        {
            printf("Error: Cannot open DCU list file %s\n", argv[2]);
            fclose(fOut);
            return -1;
        }
    }

    ModuleList = new TList;
    ConstList = new TList;
    TypeList = new TList;
    VarList = new TList;
    ResStrList = new TList;
    ProcList = new TList;

    FUnitImp = new TList;
    FTypes = new TList;
    FAddrs = new TList;
    FTypeShowStack = new TList;

    if (argc == 2)
    {
        if (FindFirst("*.dcu", iAttributes, sr) == 0)
        {
            do
            {
                printf("Unit %s\n", sr.Name.c_str());
                ScanOneDCU(sr.Name); num++;
                printf("Done\n");
            } while (FindNext(sr) == 0);

            FindClose(sr);
        }
    }
    else
    {
        while (1)
        {
            if (!fgets(dcuFilename, 256, fList)) break;
            char* q = strrchr(dcuFilename, '\n');
            if (q) *q = 0;
            printf("Unit %s\n", dcuFilename);
            ScanOneDCU(String(dcuFilename)); num++;
            printf("Done\n");
        }
        fclose(fList);
    }

    delete FUnitImp;
    delete FTypes;
    delete FAddrs;
    delete FTypeShowStack;

    printf("Total files %d\n", num);

    DWORD CurrOffset = 0;
    FILE *fIn;
    int charnum;
    long filelen;
//---------------------------------------------------------------------KB header
#ifdef NEW_VERSION
    char *KBSignature = "IDR Knowledge Base File";
#else
    char *KBSignature = "IDD Knowledge Base File";
#endif
    fwrite(KBSignature, 1, strlen(KBSignature) + 1, fOut); CurrOffset += strlen(KBSignature) + 1;
    fwrite(&IsMSIL, sizeof(IsMSIL), 1, fOut); CurrOffset += sizeof(IsMSIL);
    fwrite(&FVer, sizeof(FVer), 1, fOut); CurrOffset += sizeof(FVer);
    DWORD CRC = 0xFFFFFFFF;
    fwrite(&CRC, sizeof(CRC), 1, fOut); CurrOffset += sizeof(CRC);
    char Description[256];
    fwrite(Description, 1, 256, fOut); CurrOffset += 256;
#ifdef NEW_VERSION
    DWORD Version = 2.0;
#else
    DWORD Version = 1.0;
#endif
    fwrite(&Version, sizeof(Version), 1, fOut); CurrOffset += sizeof(Version);
    TDateTime CreateDT, LastModifyDT;
    fwrite(&CreateDT, sizeof(CreateDT), 1, fOut); CurrOffset += sizeof(CreateDT);
    fwrite(&LastModifyDT, sizeof(LastModifyDT), 1, fOut); CurrOffset += sizeof(LastModifyDT);
//-----------------------------------------------------------------------MODULES
    int ModuleCount = ModuleList->Count;
    int MaxModuleDataSize = 0;
    for (int n = 0; n < ModuleCount; n++)
    {
        ModuleInfo = (PMODULEINFO)ModuleList->Items[n];
        ModuleInfo->ID = n;
        ModuleInfo->Offset = CurrOffset;

        DWORD DataSize = 0;
        //ModuleID
        fwrite(&ModuleInfo->ModuleID, sizeof(WORD), 1, fOut);
        DataSize += sizeof(WORD);
        //Name
        DataSize += WriteString(fOut, ModuleInfo->Name);
        //Filename
        DataSize += WriteString(fOut, ModuleInfo->Filename);
        //UsesNum
        WORD UsesNum = ModuleInfo->UsesList->Count;
        fwrite(&UsesNum, sizeof(UsesNum), 1, fOut);
        DataSize += sizeof(UsesNum);
        //Uses
        for (int m = 0; m < UsesNum; m++)
        {
            bool found = false;
            for (int mm = 0; mm < ModuleCount; mm++)
            {
                PMODULEINFO modInfo = (PMODULEINFO)ModuleList->Items[mm];
                if (!modInfo->Name.AnsiCompareIC(ModuleInfo->UsesList->Strings[m]))
                {
                    fwrite(&modInfo->ModuleID, sizeof(WORD), 1, fOut);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                WORD mID = 0xFFFF;
                fwrite(&mID, sizeof(WORD), 1, fOut);
                //Вспомогательная печать для определения замкнутого множества юнитов 
                //if (!ModuleInfo->UsesList->Strings[m].Pos("."))
                //    printf("-%s\n", ModuleInfo->UsesList->Strings[m]);
            }
            DataSize += sizeof(WORD);
        }
        for (int m = 0; m < UsesNum; m++)
        {
            DataSize += WriteString(fOut, ModuleInfo->UsesList->Strings[m]);
        }

        ModuleInfo->Size = DataSize;
        if (DataSize > MaxModuleDataSize) MaxModuleDataSize = DataSize;
        CurrOffset += DataSize;
    }
    fflush(fOut);
    filelen = ftell(fOut);
    if (filelen != CurrOffset) printf("Error: MODULES\n");
//---------------------------------------------------------------------CONSTANTS
    ConstList->Sort(CompareConstsByName);
    int ConstCount = 0;
    int MaxConstDataSize = 0;
    WORD PrevModId = 0xFFFF;
    String PrevName = "";
    for (int n = 0; n < ConstList->Count; n++)
    {
        PCONSTINFO constInfo = (PCONSTINFO)ConstList->Items[n];
        constInfo->Skip = false;
        if (constInfo->Name == PrevName && constInfo->ModuleID == PrevModId)
        {
            constInfo->Skip = true;
            continue;
        }

        constInfo->ID = ConstCount;
        constInfo->Offset = CurrOffset;
        PrevModId = constInfo->ModuleID;
        PrevName = constInfo->Name;

        DWORD DataSize = 0;
        //ModuleID
        fwrite(&constInfo->ModuleID, sizeof(constInfo->ModuleID), 1, fOut);
        DataSize += sizeof(constInfo->ModuleID);
        //Name
        DataSize += WriteString(fOut, constInfo->Name);
        //Type
        fwrite(&constInfo->Type, sizeof(constInfo->Type), 1, fOut);
        DataSize += sizeof(constInfo->Type);
        //TypeDef
        DataSize += WriteString(fOut, constInfo->TypeDef);
        //Value
        DataSize += WriteString(fOut, constInfo->Value);
        //Не будем дампить константы с внутренними именами
        if (constInfo->Name.Pos("_NV_") == 1) constInfo->RTTISz = 0;
        //DumpTotal
        DWORD DumpTotal = 0;
        if (constInfo->RTTISz)
        {
            fIn = 0;
            for (int m = 0; m < ModuleList->Count; m++)
            {
                ModuleInfo = (PMODULEINFO)ModuleList->Items[m];
                if (ModuleInfo->ModuleID == constInfo->ModuleID)
                {
                    fIn = fopen(ModuleInfo->Filename.c_str(), "rb");
                    break;
                }
            }
            if (fIn)
            {
                //Dump
                DumpTotal += constInfo->RTTISz;
                //Relocs
                DumpTotal += constInfo->RTTISz;
                //Fixups
                DumpTotal += WriteFixups(0, constInfo->Fixups);
            }
        }
        DumpTotal += sizeof(constInfo->RTTISz); //DumpSz
        DWORD FixupNum = constInfo->Fixups->Count;
        DumpTotal += sizeof(FixupNum);          //FixupNum

        fwrite(&DumpTotal, sizeof(DumpTotal), 1, fOut);
        DataSize += sizeof(DumpTotal);
        //DumpSz
        fwrite(&constInfo->RTTISz, sizeof(constInfo->RTTISz), 1, fOut);
        DataSize += sizeof(constInfo->RTTISz);
        //FixupNum
        fwrite(&FixupNum, sizeof(FixupNum), 1, fOut);
        DataSize += sizeof(FixupNum);

        if (constInfo->RTTISz)
        {
            if (fIn)
            {
                //Dump
                DataSize += WriteDump(fIn, fOut, constInfo->RTTIOfs, constInfo->RTTISz);
                fclose(fIn);
                //Relocs
                DataSize += WriteRelocs(fOut, constInfo->Fixups, constInfo->RTTISz, constInfo->Name);
                //Fixups
                DataSize += WriteFixups(fOut, constInfo->Fixups);
            }
        }
        constInfo->Size = DataSize;
        if (DataSize > MaxConstDataSize) MaxConstDataSize = DataSize;
        CurrOffset += DataSize;
        ConstCount++;
    }
    fflush(fOut);
    filelen = ftell(fOut);
    if (filelen != CurrOffset) printf("Error: CONSTANTS\n");
//-------------------------------------------------------------------------TYPES
    int TypeCount = TypeList->Count;
    int MaxTypeDataSize = 0;
    for (int n = 0; n < TypeCount; n++)
    {
        //TypeOffsets[n].DataOffset = CurrOffset;
        PTYPEINFO typeInfo = (PTYPEINFO)TypeList->Items[n];
        typeInfo->ID = n;
        typeInfo->Offset = CurrOffset;
        DWORD DataSize = 0;
#ifdef NEW_VERSION
        //Size
        fwrite(&typeInfo->Size, sizeof(typeInfo->Size), 1, fOut);
        DataSize += sizeof(typeInfo->Size);
#endif
        //ModuleID
        fwrite(&typeInfo->ModuleID, sizeof(typeInfo->ModuleID), 1, fOut);
        DataSize += sizeof(typeInfo->ModuleID);
        //Name
        DataSize += WriteString(fOut, typeInfo->Name);
        //Kind
        fwrite(&typeInfo->Kind, sizeof(typeInfo->Kind), 1, fOut);
        DataSize += sizeof(typeInfo->Kind);
        //VMCnt
        fwrite(&typeInfo->VMCnt, sizeof(typeInfo->VMCnt), 1, fOut);
        DataSize += sizeof(typeInfo->VMCnt);
        //Decl
        DataSize += WriteString(fOut, typeInfo->Decl);
        //Не будем дампить типы с внутренними именами
        if (typeInfo->Name.Pos("_NT_") == 1) typeInfo->RTTISz = 0;
        //DumpTotal
        DWORD DumpTotal = 0;
        if (typeInfo->RTTISz)
        {
            fIn = 0;
            for (int m = 0; m < ModuleList->Count; m++)
            {
                ModuleInfo = (PMODULEINFO)ModuleList->Items[m];
                if (ModuleInfo->ModuleID == typeInfo->ModuleID)
                {
                    fIn = fopen(ModuleInfo->Filename.c_str(), "rb");
                    break;
                }
            }
            if (fIn)
            {
                //Dump
                DumpTotal += typeInfo->RTTISz;
                //Relocs
                DumpTotal += typeInfo->RTTISz;
                //Fixups
                DumpTotal += WriteFixups(0, typeInfo->Fixups);
            }
        }
        DumpTotal += sizeof(typeInfo->RTTISz);  //DumpSz
        DWORD FixupNum = typeInfo->Fixups->Count;
        DumpTotal += sizeof(FixupNum);          //FixupNum
        
        fwrite(&DumpTotal, sizeof(DumpTotal), 1, fOut);
        DataSize += sizeof(DumpTotal);
        //DumpSz
        fwrite(&typeInfo->RTTISz, sizeof(typeInfo->RTTISz), 1, fOut);
        DataSize += sizeof(typeInfo->RTTISz);
        //FixupNum
        fwrite(&FixupNum, sizeof(FixupNum), 1, fOut);
        DataSize += sizeof(FixupNum);

        if (typeInfo->RTTISz)
        {
            if (fIn)
            {
                //Dump
                DataSize += WriteDump(fIn, fOut, typeInfo->RTTIOfs, typeInfo->RTTISz);
                fclose(fIn);
                //Relocs
                DataSize += WriteRelocs(fOut, typeInfo->Fixups, typeInfo->RTTISz, typeInfo->Name);
                //Fixups
                DataSize += WriteFixups(fOut, typeInfo->Fixups);
            }
        }
        //FieldsTotal
        WORD FieldsNum = typeInfo->Fields->Count;
        DWORD FieldsTotal = 0;
        for (int m = 0; m < FieldsNum; m++)
        {
            PLOCALDECLINFO linfo = (PLOCALDECLINFO)typeInfo->Fields->Items[m];
            FieldsTotal += sizeof(linfo->Scope);
            FieldsTotal += sizeof(linfo->Ndx);
            FieldsTotal += sizeof(linfo->Case);
            FieldsTotal += WriteString(0, linfo->Name);
            FieldsTotal += WriteString(0, linfo->TypeDef);
        }
        FieldsTotal += sizeof(FieldsNum);   //FieldsNum

        fwrite(&FieldsTotal, sizeof(FieldsTotal), 1, fOut);
        DataSize += sizeof(FieldsTotal);
        //FieldsNum
        fwrite(&FieldsNum, sizeof(FieldsNum), 1, fOut);
        DataSize += sizeof(FieldsNum);
        //Fields
        for (int m = 0; m < FieldsNum; m++)
        {
            PLOCALDECLINFO linfo = (PLOCALDECLINFO)typeInfo->Fields->Items[m];
            fwrite(&linfo->Scope, sizeof(linfo->Scope), 1, fOut);
            DataSize += sizeof(linfo->Scope);
            fwrite(&linfo->Ndx, sizeof(linfo->Ndx), 1, fOut);
            DataSize += sizeof(linfo->Ndx);
            fwrite(&linfo->Case, sizeof(linfo->Case), 1, fOut);
            DataSize += sizeof(linfo->Case);
            DataSize += WriteString(fOut, linfo->Name);
            DataSize += WriteString(fOut, linfo->TypeDef);
        }
        //PropsTotal
        WORD PropsNum = typeInfo->Properties->Count;
        DWORD PropsTotal = 0;
        for (int m = 0; m < PropsNum; m++)
        {
            if (typeInfo->Kind == drClassDef || typeInfo->Kind == drInterfaceDef)
            {
                PPROPERTYINFO pinfo = (PPROPERTYINFO)typeInfo->Properties->Items[m];
                PropsTotal += sizeof(pinfo->Scope);
                PropsTotal += sizeof(pinfo->Index);
                PropsTotal += sizeof(pinfo->DispId);
                PropsTotal += WriteString(0, pinfo->Name);
                PropsTotal += WriteString(0, pinfo->TypeDef);
                PropsTotal += WriteString(0, pinfo->ReadName);
                PropsTotal += WriteString(0, pinfo->WriteName);
                PropsTotal += WriteString(0, pinfo->StoredName);
            }
        }
        PropsTotal += sizeof(PropsNum); //PropsNum

        fwrite(&PropsTotal, sizeof(PropsTotal), 1, fOut);
        DataSize += sizeof(PropsTotal);
        //PropsNum
        fwrite(&PropsNum, sizeof(PropsNum), 1, fOut);
        DataSize += sizeof(PropsNum);
        //Props
        for (int m = 0; m < PropsNum; m++)
        {
            if (typeInfo->Kind == drClassDef || typeInfo->Kind == drInterfaceDef)
            {
                PPROPERTYINFO pinfo = (PPROPERTYINFO)typeInfo->Properties->Items[m];
                fwrite(&pinfo->Scope, sizeof(pinfo->Scope), 1, fOut);
                DataSize += sizeof(pinfo->Scope);
                fwrite(&pinfo->Index, sizeof(pinfo->Index), 1, fOut);
                DataSize += sizeof(pinfo->Index);
                fwrite(&pinfo->DispId, sizeof(pinfo->DispId), 1, fOut);
                DataSize += sizeof(pinfo->DispId);
                DataSize += WriteString(fOut, pinfo->Name);
                DataSize += WriteString(fOut, pinfo->TypeDef);
                DataSize += WriteString(fOut, pinfo->ReadName);
                DataSize += WriteString(fOut, pinfo->WriteName);
                DataSize += WriteString(fOut, pinfo->StoredName);
            }
        }
        //MethodsTotal
        WORD MethodsNum = typeInfo->Methods->Count;
        DWORD MethodsTotal = 0;
        for (int m = 0; m < MethodsNum; m++)
        {
            PMETHODDECLINFO minfo = (PMETHODDECLINFO)typeInfo->Methods->Items[m];
            MethodsTotal += sizeof(minfo->Scope);
            MethodsTotal += sizeof(minfo->MethodKind);
            MethodsTotal += WriteString(0, minfo->Prototype);
        }
        MethodsTotal += sizeof(MethodsNum); //MethodsNum

        fwrite(&MethodsTotal, sizeof(MethodsTotal), 1, fOut);
        DataSize += sizeof(MethodsTotal);
        //MethodsNum
        fwrite(&MethodsNum, sizeof(MethodsNum), 1, fOut);
        DataSize += sizeof(MethodsNum);
        //Methods
        for (int m = 0; m < MethodsNum; m++)
        {
            PMETHODDECLINFO minfo = (PMETHODDECLINFO)typeInfo->Methods->Items[m];
            fwrite(&minfo->Scope, sizeof(minfo->Scope), 1, fOut);
            DataSize += sizeof(minfo->Scope);
            fwrite(&minfo->MethodKind, sizeof(minfo->MethodKind), 1, fOut);
            DataSize += sizeof(minfo->MethodKind);
            DataSize += WriteString(fOut, minfo->Prototype);
        }
        typeInfo->Size = DataSize;
        if (DataSize > MaxTypeDataSize) MaxTypeDataSize = DataSize;
        CurrOffset += DataSize;
    }
    fflush(fOut);
    filelen = ftell(fOut);
    if (filelen != CurrOffset) printf("Error: TYPES\n");
//--------------------------------------------------------------------------VARS
    int VarCount = VarList->Count;
    int MaxVarDataSize = 0;
    for (int n = 0; n < VarCount; n++)
    {
        PVARINFO vInfo = (PVARINFO)VarList->Items[n];
        vInfo->ID = n;
        vInfo->Offset = CurrOffset;

        DWORD DataSize = 0;
        //ModuleID
        fwrite(&vInfo->ModuleID, sizeof(vInfo->ModuleID), 1, fOut);
        DataSize += sizeof(vInfo->ModuleID);
        //Name
        DataSize += WriteString(fOut, vInfo->Name);
        //Type
        fwrite(&vInfo->Type, sizeof(vInfo->Type), 1, fOut);
        DataSize += sizeof(vInfo->Type);
        //TypeDef
        DataSize += WriteString(fOut, vInfo->TypeDef);
        //AbsName
        DataSize += WriteString(fOut, vInfo->AbsName);

        vInfo->Size = DataSize;
        if (DataSize > MaxVarDataSize) MaxVarDataSize = DataSize;
        CurrOffset += DataSize;
    }
    fflush(fOut);
    filelen = ftell(fOut);
    if (filelen != CurrOffset) printf("Error: VARS\n");
//--------------------------------------------------------------RESOURCE STRINGS
    int ResStrCount = ResStrList->Count;
    int MaxResStrDataSize = 0;
    BYTE ResStrBuf[1024];
    for (int n = 0; n < ResStrCount; n++)
    {
        PRESSTRINFO rsInfo = (PRESSTRINFO)ResStrList->Items[n];
        rsInfo->ID = n;
        rsInfo->Offset = CurrOffset;

        DWORD DataSize = 0;
        //ModuleID
        fwrite(&rsInfo->ModuleID, sizeof(rsInfo->ModuleID), 1, fOut);
        DataSize += sizeof(rsInfo->ModuleID);
        //Name
        DataSize += WriteString(fOut, rsInfo->Name);
        //TypeDef
        DataSize += WriteString(fOut, rsInfo->TypeDef);
        //Context
        if (rsInfo->DumpSz)
        {
            fIn = 0;
            for (int m = 0; m < ModuleList->Count; m++)
            {
                ModuleInfo = (PMODULEINFO)ModuleList->Items[m];
                if (ModuleInfo->ModuleID == rsInfo->ModuleID)
                {
                    fIn = fopen(ModuleInfo->Filename.c_str(), "rb");
                    break;
                }
            }
            if (fIn)
            {
                //Context
                fseek(fIn, rsInfo->DumpOfs, SEEK_SET);
                fread(ResStrBuf, 1, rsInfo->DumpSz, fIn);
                fclose(fIn);
                WORD ResStrLen = *((WORD*)(ResStrBuf + 4));
                rsInfo->Context = String((char*)(ResStrBuf + 8), ResStrLen);
                DataSize += WriteString(fOut, rsInfo->Context);
            }
        }
        rsInfo->Size = DataSize;
        if (DataSize > MaxResStrDataSize) MaxResStrDataSize = DataSize;
        CurrOffset += DataSize;
    }
    fflush(fOut);
    filelen = ftell(fOut);
    if (filelen != CurrOffset) printf("Error: RESOURCE STRINGS\n");
//--------------------------------------------------------------------PROCEDURES
    int ProcCount = ProcList->Count;
    int MaxProcDataSize = 0;
    for (int n = 0; n < ProcCount; n++)
    {
        DWORD DataSize = 0;
        PPROCDECLINFO pInfo = (PPROCDECLINFO)ProcList->Items[n];
        pInfo->ID = n;
        pInfo->Offset = CurrOffset;
        //ModuleID
        fwrite(&pInfo->ModuleID, sizeof(pInfo->ModuleID), 1, fOut);
        DataSize += sizeof(pInfo->ModuleID);
        //Name
        DataSize += WriteString(fOut, pInfo->Name);
        //Embedded
        fwrite(&pInfo->Embedded, sizeof(pInfo->Embedded), 1, fOut);
        DataSize += sizeof(pInfo->Embedded);
        //DumpType
        fwrite(&pInfo->DumpType, sizeof(pInfo->DumpType), 1, fOut);
        DataSize += sizeof(pInfo->DumpType);
        //MethodKind
        fwrite(&pInfo->MethodKind, sizeof(pInfo->MethodKind), 1, fOut);
        DataSize += sizeof(pInfo->MethodKind);
        //CallKind
        fwrite(&pInfo->CallKind, sizeof(pInfo->CallKind), 1, fOut);
        DataSize += sizeof(pInfo->CallKind);
        //VProc
        fwrite(&pInfo->VProc, sizeof(pInfo->VProc), 1, fOut);
        DataSize += sizeof(pInfo->VProc);
        //TypeDef
        DataSize += WriteString(fOut, pInfo->TypeDef);
        //DumpTotal
        DWORD DumpTotal = 0;
        if (pInfo->DumpSz)
        {
            fIn = 0;
            for (int m = 0; m < ModuleList->Count; m++)
            {
                ModuleInfo = (PMODULEINFO)ModuleList->Items[m];
                if (ModuleInfo->ModuleID == pInfo->ModuleID)
                {
                    fIn = fopen(ModuleInfo->Filename.c_str(), "rb");
                    break;
                }
            }
            if (fIn)
            {
                //Dump
                DumpTotal += pInfo->DumpSz;
                //Relocs
                DumpTotal += pInfo->DumpSz;
                //Fixups
                DumpTotal += WriteFixups(0, pInfo->Fixups);
            }
        }
        DumpTotal += sizeof(pInfo->DumpSz); //DumpSz
        DWORD FixupNum = pInfo->Fixups->Count;
        DumpTotal += sizeof(FixupNum);      //FixupNum
        
        fwrite(&DumpTotal, sizeof(DumpTotal), 1, fOut);
        DataSize += sizeof(DumpTotal);
        //DumpSz
        fwrite(&pInfo->DumpSz, sizeof(pInfo->DumpSz), 1, fOut);
        DataSize += sizeof(pInfo->DumpSz);
        //FixupNum
        fwrite(&FixupNum, sizeof(FixupNum), 1, fOut);
        DataSize += sizeof(FixupNum);
        
        if (pInfo->DumpSz)
        {
            if (fIn)
            {
                //Dump
                DataSize += WriteDump(fIn, fOut, pInfo->DumpOfs, pInfo->DumpSz);
                fclose(fIn);
                //Relocs
                DataSize += WriteRelocs(fOut, pInfo->Fixups, pInfo->DumpSz, pInfo->Name);
                //Fixups
                DataSize += WriteFixups(fOut, pInfo->Fixups);
            }
        }
        //ArgsTotal
        WORD ArgsNum = pInfo->Args->Count;
        DWORD ArgsTotal = 0;
        for (int m = 0; m < ArgsNum; m++)
        {
            PLOCALDECLINFO lInfo = (PLOCALDECLINFO)pInfo->Args->Items[m];
            ArgsTotal += sizeof(lInfo->Tag);
            ArgsTotal += sizeof(lInfo->LocFlags);
            ArgsTotal += sizeof(lInfo->Ndx);
            ArgsTotal += WriteString(0, lInfo->Name);
            ArgsTotal += WriteString(0, lInfo->TypeDef);
        }
        ArgsTotal += sizeof(ArgsNum);   //ArgsNum
        
        fwrite(&ArgsTotal, sizeof(ArgsTotal), 1, fOut);
        DataSize += sizeof(ArgsTotal);
        //ArgsNum
        fwrite(&ArgsNum, sizeof(ArgsNum), 1, fOut);
        DataSize += sizeof(ArgsNum);
        //Args
        for (int m = 0; m < ArgsNum; m++)
        {
            PLOCALDECLINFO lInfo = (PLOCALDECLINFO)pInfo->Args->Items[m];

            fwrite(&lInfo->Tag, sizeof(lInfo->Tag), 1, fOut);
            DataSize += sizeof(lInfo->Tag);
            fwrite(&lInfo->LocFlags, sizeof(lInfo->LocFlags), 1, fOut);
            DataSize += sizeof(lInfo->LocFlags);
            fwrite(&lInfo->Ndx, sizeof(lInfo->Ndx), 1, fOut);
            DataSize += sizeof(lInfo->Ndx);
            DataSize += WriteString(fOut, lInfo->Name);
            DataSize += WriteString(fOut, lInfo->TypeDef);
        }
        //LocalsTotal
        WORD LocalsNum = pInfo->Locals->Count;
        DWORD LocalsTotal = 0;
        for (int m = 0; m < LocalsNum; m++)
        {
            PLOCALDECLINFO lInfo = (PLOCALDECLINFO)pInfo->Locals->Items[m];
            LocalsTotal += sizeof(lInfo->Tag);
            LocalsTotal += sizeof(lInfo->LocFlags);
            LocalsTotal += sizeof(lInfo->Ndx);
            LocalsTotal += WriteString(0, lInfo->Name);
            LocalsTotal += WriteString(0, lInfo->TypeDef);
            LocalsTotal += WriteString(0, lInfo->AbsName);
        }
        LocalsTotal += sizeof(LocalsNum);   //LocalsNum
        
        fwrite(&LocalsTotal, sizeof(LocalsTotal), 1, fOut);
        DataSize += sizeof(LocalsTotal);
        //LocalsNum
        fwrite(&LocalsNum, sizeof(LocalsNum), 1, fOut);
        DataSize += sizeof(LocalsNum);
        //Locals
        for (int m = 0; m < LocalsNum; m++)
        {
            PLOCALDECLINFO lInfo = (PLOCALDECLINFO)pInfo->Locals->Items[m];

            fwrite(&lInfo->Tag, sizeof(lInfo->Tag), 1, fOut);
            DataSize += sizeof(lInfo->Tag);
            fwrite(&lInfo->LocFlags, sizeof(lInfo->LocFlags), 1, fOut);
            DataSize += sizeof(lInfo->LocFlags);
            fwrite(&lInfo->Ndx, sizeof(lInfo->Ndx), 1, fOut);
            DataSize += sizeof(lInfo->Ndx);
            DataSize += WriteString(fOut, lInfo->Name);
            DataSize += WriteString(fOut, lInfo->TypeDef);
            DataSize += WriteString(fOut, lInfo->AbsName);
        }
        pInfo->Size = DataSize;
        if (DataSize > MaxProcDataSize) MaxProcDataSize = DataSize;
        CurrOffset += DataSize;
        fflush(fOut);
        filelen = ftell(fOut);
        if (filelen != CurrOffset) printf("Error: PROCEDURES\n");
    }
    fflush(fOut);
    filelen = ftell(fOut);
    if (filelen != CurrOffset) printf("Error: PROCEDURES\n");
//----------------------------------------------------------------Module Section
    fwrite(&ModuleCount, sizeof(ModuleCount), 1, fOut);
    fwrite(&MaxModuleDataSize, sizeof(MaxModuleDataSize), 1, fOut);

    POFFSETSINFO ModuleOffsets = new OFFSETSINFO[ModuleCount];
    for (int n = 0; n < ModuleCount; n++)
    {
        ModuleInfo = (PMODULEINFO)ModuleList->Items[n];
        ModuleOffsets[n].Offset = ModuleInfo->Offset;
        ModuleOffsets[n].Size = ModuleInfo->Size;
    }
    ModuleList->Sort(CompareModulesByID);
    for (int n = 0; n < ModuleCount; n++)
    {
        ModuleInfo = (PMODULEINFO)ModuleList->Items[n];
        ModuleOffsets[n].ModId = ModuleInfo->ID;
    }
    ModuleList->Sort(CompareModulesByName);
    for (int n = 0; n < ModuleCount; n++)
    {
        ModuleInfo = (PMODULEINFO)ModuleList->Items[n];
        ModuleOffsets[n].NamId = ModuleInfo->ID;
    }
    fwrite(ModuleOffsets, sizeof(OFFSETSINFO), ModuleCount, fOut);
    delete[] ModuleOffsets;
//-----------------------------------------------------------------Const Section
    fwrite(&ConstCount, sizeof(ConstCount), 1, fOut);
    fwrite(&MaxConstDataSize, sizeof(MaxConstDataSize), 1, fOut);

    POFFSETSINFO ConstOffsets = new OFFSETSINFO[ConstCount];
    int cn = 0;
    for (int n = 0; n < ConstList->Count; n++)
    {
        PCONSTINFO constInfo = (PCONSTINFO)ConstList->Items[n];
        if (constInfo->Skip) continue;

        ConstOffsets[cn].Offset = constInfo->Offset;
        ConstOffsets[cn].Size = constInfo->Size;
        cn++;
    }
    ConstList->Sort(CompareConstsByID);
    cn = 0;
    for (int n = 0; n < ConstList->Count; n++)
    {
        PCONSTINFO constInfo = (PCONSTINFO)ConstList->Items[n];
        if (constInfo->Skip) continue;

        ConstOffsets[cn].ModId = constInfo->ID;
        cn++;
    }
    ConstList->Sort(CompareConstsByName);
    cn = 0;
    for (int n = 0; n < ConstList->Count; n++)
    {
        PCONSTINFO constInfo = (PCONSTINFO)ConstList->Items[n];
        if (constInfo->Skip) continue;

        ConstOffsets[cn].NamId = constInfo->ID;
        cn++;
    }
    fwrite(ConstOffsets, sizeof(OFFSETSINFO), ConstCount, fOut);
    delete[] ConstOffsets;
//------------------------------------------------------------------Type Section
    fwrite(&TypeCount, sizeof(TypeCount), 1, fOut);
    fwrite(&MaxTypeDataSize, sizeof(MaxTypeDataSize), 1, fOut);

    POFFSETSINFO TypeOffsets = new OFFSETSINFO[TypeCount];
    for (int n = 0; n < TypeCount; n++)
    {
        PTYPEINFO typeInfo = (PTYPEINFO)TypeList->Items[n];
        TypeOffsets[n].Offset = typeInfo->Offset;
        TypeOffsets[n].Size = typeInfo->Size;
    }
    TypeList->Sort(CompareTypesByID);
    for (int n = 0; n < TypeCount; n++)
    {
        PTYPEINFO typeInfo = (PTYPEINFO)TypeList->Items[n];
        TypeOffsets[n].ModId = typeInfo->ID;
    }
    TypeList->Sort(CompareTypesByName);
    for (int n = 0; n < TypeCount; n++)
    {
        PTYPEINFO typeInfo = (PTYPEINFO)TypeList->Items[n];
        TypeOffsets[n].NamId = typeInfo->ID;
    }
    fwrite(TypeOffsets, sizeof(OFFSETSINFO), TypeCount, fOut);
    delete[] TypeOffsets;
//-------------------------------------------------------------------Var Section
    fwrite(&VarCount, sizeof(VarCount), 1, fOut);
    fwrite(&MaxVarDataSize, sizeof(MaxVarDataSize), 1, fOut);

    POFFSETSINFO VarOffsets = new OFFSETSINFO[VarCount];
    for (int n = 0; n < VarCount; n++)
    {
        PVARINFO vInfo = (PVARINFO)VarList->Items[n];
        VarOffsets[n].Offset = vInfo->Offset;
        VarOffsets[n].Size = vInfo->Size;
    }
    VarList->Sort(CompareVarsByID);
    for (int n = 0; n < VarCount; n++)
    {
        PVARINFO vInfo = (PVARINFO)VarList->Items[n];
        VarOffsets[n].ModId = vInfo->ID;
    }
    VarList->Sort(CompareVarsByName);
    for (int n = 0; n < VarCount; n++)
    {
        PVARINFO vInfo = (PVARINFO)VarList->Items[n];
        VarOffsets[n].NamId = vInfo->ID;
    }
    fwrite(VarOffsets, sizeof(OFFSETSINFO), VarCount, fOut);
    delete[] VarOffsets;
//----------------------------------------------------------------ResStr Section
    fwrite(&ResStrCount, sizeof(ResStrCount), 1, fOut);
    fwrite(&MaxResStrDataSize, sizeof(MaxResStrDataSize), 1, fOut);

    POFFSETSINFO ResStrOffsets = new OFFSETSINFO[ResStrCount];
    for (int n = 0; n < ResStrCount; n++)
    {
        PRESSTRINFO rsInfo = (PRESSTRINFO)ResStrList->Items[n];
        ResStrOffsets[n].Offset = rsInfo->Offset;
        ResStrOffsets[n].Size = rsInfo->Size;
    }
    ResStrList->Sort(CompareResStrsByID);
    for (int n = 0; n < ResStrCount; n++)
    {
        PRESSTRINFO rsInfo = (PRESSTRINFO)ResStrList->Items[n];
        ResStrOffsets[n].ModId = rsInfo->ID;
    }
    ResStrList->Sort(CompareResStrsByName);
    for (int n = 0; n < ResStrCount; n++)
    {
        PRESSTRINFO rsInfo = (PRESSTRINFO)ResStrList->Items[n];
        ResStrOffsets[n].NamId = rsInfo->ID;
    }
    fwrite(ResStrOffsets, sizeof(OFFSETSINFO), ResStrCount, fOut);
    delete[] ResStrOffsets;
//------------------------------------------------------------------Proc Section
    fwrite(&ProcCount, sizeof(ProcCount), 1, fOut);
    fwrite(&MaxProcDataSize, sizeof(MaxProcDataSize), 1, fOut);

    POFFSETSINFO ProcOffsets = new OFFSETSINFO[ProcCount];
    for (int n = 0; n < ProcCount; n++)
    {
        PPROCDECLINFO pInfo = (PPROCDECLINFO)ProcList->Items[n];
        ProcOffsets[n].Offset = pInfo->Offset;
        ProcOffsets[n].Size = pInfo->Size;
    }
    ProcList->Sort(CompareProcsByID);
    for (int n = 0; n < ProcCount; n++)
    {
        PPROCDECLINFO pInfo = (PPROCDECLINFO)ProcList->Items[n];
        ProcOffsets[n].ModId = pInfo->ID;
    }
    ProcList->Sort(CompareProcsByName);
    for (int n = 0; n < ProcCount; n++)
    {
        PPROCDECLINFO pInfo = (PPROCDECLINFO)ProcList->Items[n];
        ProcOffsets[n].NamId = pInfo->ID;
    }
    fwrite(ProcOffsets, sizeof(OFFSETSINFO), ProcCount, fOut);
    delete[] ProcOffsets;
//----------------------------------------------------------------Sections begin
    fwrite(&CurrOffset, sizeof(CurrOffset), 1, fOut);
    fclose(fOut);
    if (fLog) fclose(fLog);

    ModuleList->Clear();

    delete ModuleList;
    delete ConstList;
    delete TypeList;
    delete VarList;
    delete ResStrList;
    delete ProcList;
    return 0;
}
//------------------------------------------------------------------------------
