//---------------------------------------------------------------------------
#include <stdio.h>
#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------
//Информация о смещениях имен и данных
typedef struct
{
    DWORD	Offset;
    DWORD	Size;
    int     ModID;
    int     NamID;
} OFFSETSINFO, *POFFSETSINFO;
//---------------------------------------------------------------------------
String CallKindName[5] = {"fastcall", "cdecl", "pascal", "stdcall", "safecall"};
#pragma argsused
int main(int argc, char* argv[])
{
    //Test Result KB
    if (argc < 2)
    {
        printf("Usage TestKB KBName\n");
        return -1;
    }
    //Read and print Modules
    FILE *fIn = fopen(argv[1], "rb");
    if (!fIn)
    {
        printf("Cannot open KB file %s\n", argv[1]);
        return -1;
    }
    FILE *fOut = fopen("test.out", "wt+");
    if (!fOut)
    {
        fclose(fIn);
        printf("Cannot open test.out\n");
        return -1;
    }
    //read offset of Sections
    fseek(fIn, -4L, SEEK_END);
    long SectionsOffset, DataOffset;
    DWORD Size, DataSize, DumpSize, DumpTotal, FixupNum, FixupOffset;
    WORD NameLen, ModuleID;
    BYTE *TmpBuf, *p, FixupType;
    fread(&SectionsOffset, sizeof(SectionsOffset), 1, fIn);
    fseek(fIn, SectionsOffset, SEEK_SET);

    //Module Section
    int ModuleCount;
    fread(&ModuleCount, sizeof(ModuleCount), 1, fIn);
    int MaxModuleDataSize;
    fread(&MaxModuleDataSize, sizeof(MaxModuleDataSize), 1, fIn);
    POFFSETSINFO ModuleOffsets = new OFFSETSINFO[ModuleCount];
    fread(ModuleOffsets, sizeof(OFFSETSINFO), ModuleCount, fIn);

    //Const Section
    int ConstCount;
    fread(&ConstCount, sizeof(ConstCount), 1, fIn);
    int MaxConstDataSize;
    fread(&MaxConstDataSize, sizeof(MaxConstDataSize), 1, fIn);
    POFFSETSINFO ConstOffsets = new OFFSETSINFO[ConstCount];
    fread(ConstOffsets, sizeof(OFFSETSINFO), ConstCount, fIn);

    //Type Section
    int TypeCount;
    fread(&TypeCount, sizeof(TypeCount), 1, fIn);
    int MaxTypeDataSize;
    fread(&MaxTypeDataSize, sizeof(MaxTypeDataSize), 1, fIn);
    POFFSETSINFO TypeOffsets = new OFFSETSINFO[TypeCount];
    fread(TypeOffsets, sizeof(OFFSETSINFO), TypeCount, fIn);

    //Var Section
    int VarCount;
    fread(&VarCount, sizeof(VarCount), 1, fIn);
    int MaxVarDataSize;
    fread(&MaxVarDataSize, sizeof(MaxVarDataSize), 1, fIn);
    POFFSETSINFO VarOffsets = new OFFSETSINFO[VarCount];
    fread(VarOffsets, sizeof(OFFSETSINFO), VarCount, fIn);

    //ResStr Section
    int ResStrCount;
    fread(&ResStrCount, sizeof(ResStrCount), 1, fIn);
    int MaxResStrDataSize;
    fread(&MaxResStrDataSize, sizeof(MaxResStrDataSize), 1, fIn);
    POFFSETSINFO ResStrOffsets = new OFFSETSINFO[ResStrCount];
    fread(ResStrOffsets, sizeof(OFFSETSINFO), ResStrCount, fIn);

    //Proc Section
    int ProcCount;
    fread(&ProcCount, sizeof(ProcCount), 1, fIn);
    int MaxProcDataSize;
    fread(&MaxProcDataSize, sizeof(MaxProcDataSize), 1, fIn);
    POFFSETSINFO ProcOffsets = new OFFSETSINFO[ProcCount];
    fread(ProcOffsets, sizeof(OFFSETSINFO), ProcCount, fIn);

    int ID;
    //Test Module Section
    TmpBuf = new BYTE[MaxModuleDataSize];
    for (int n = 0; n < ModuleCount; n++)
    {
        ID = ModuleOffsets[n].NamID;
        //Читаем данные
        fseek(fIn, ModuleOffsets[ID].Offset, SEEK_SET);
        fread(TmpBuf, 1, ModuleOffsets[ID].Size, fIn);
        p = TmpBuf;
        //ID
        ModuleID = *((WORD*)p); p += sizeof(WORD);
        //Name
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "MODULE %s ID%d\n", p, ModuleID);
        p += NameLen + 1;
        //Filename
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "  FILENAME=%s\n", p);
        p += NameLen + 1;
        //Uses
        WORD UsesNum = *((WORD*)p); p += sizeof(WORD);
        for (int m = 0; m < UsesNum; m++)
        {
            if (!m) fprintf(fOut, "  USES\n");
            ModuleID = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, " %d", ModuleID);
            //NameLen = *((WORD*)p); p += sizeof(WORD);
            //fprintf(fOut, "  %s\n", p);
            //p += NameLen + 1;
        }
        if (UsesNum) fprintf(fOut, "\n");
        for (int m = 0; m < UsesNum; m++)
        {
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, "  %s\n", p);
            p += NameLen + 1;
        }
    }
    delete[] TmpBuf;

    //Test Const Section
    TmpBuf = new BYTE[MaxConstDataSize];
    for (int n = 0; n < ConstCount; n++)
    {
        ID = ConstOffsets[n].NamID;
        fseek(fIn, ConstOffsets[ID].Offset, SEEK_SET);
        fread(TmpBuf, 1, ConstOffsets[ID].Size, fIn);
        p = TmpBuf;
        //ModuleID
        ModuleID = *((WORD*)p); p += sizeof(WORD);
        //Name
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "CONST %s (ID%d)", p, ModuleID);
        p += NameLen + 1;
        //ConstType
        BYTE ConstType = *p; p++;
        fprintf(fOut, "(TYPE=%c)\n", ConstType);
        //TypeDef
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "  TYPEDEF=%s\n", p);
        p += NameLen + 1;
        //Value
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "  VALUE=%s\n", p);
        p += NameLen + 1;
        //DumpTotal
        DumpTotal = *((DWORD*)p); p += sizeof(DWORD);
        //DumpSz
        DumpSize = *((DWORD*)p); p += sizeof(DWORD);
        //FixupNum
        FixupNum = *((DWORD*)p); p += sizeof(DWORD);
        //Dump
        if (DumpSize)
        {
            fprintf(fOut, "  DUMP=");
            for (int m = 0; m < DumpSize; m++)
            {
                if (m) fprintf(fOut, " ");
                fprintf(fOut, "%02X", *p);
                p++;
            }
            fprintf(fOut, "\n");
            fprintf(fOut, "  RELOCS=");
            for (int m = 0; m < DumpSize; m++)
            {
                if (m) fprintf(fOut, " ");
                fprintf(fOut, "%02X", *p);
                p++;
            }
            fprintf(fOut, "\n");
            //Fixups
            for (int m = 0; m < FixupNum; m++)
            {
                if (!m) fprintf(fOut, "  FIXUPS\n");
                //Type
                FixupType = *p; p++;
                fprintf(fOut, "  %c", FixupType);
                //Offset
                FixupOffset = *((DWORD*)p); p += sizeof(DWORD);
                fprintf(fOut, " %lX", FixupOffset);
                //Name
                NameLen = *((WORD*)p); p += sizeof(WORD);
                fprintf(fOut, " %s", p);
                p += NameLen + 1;
                fprintf(fOut, "\n");
            }
        }
    }
    delete[] TmpBuf;

    //Test Type Section
    TmpBuf = new BYTE[MaxTypeDataSize];
    for (int n = 0; n < TypeCount; n++)
    {
        ID = TypeOffsets[n].NamID;
        fseek(fIn, TypeOffsets[ID].Offset, SEEK_SET);
        fread(TmpBuf, 1, TypeOffsets[ID].Size, fIn);
        p = TmpBuf;
        //Size
        Size = *((DWORD*)p); p += sizeof(DWORD);
        //ModuleID
        ModuleID = *((WORD*)p); p += sizeof(WORD);
        //Name
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "TYPE %s (ID%d, Size=%d)", p, ModuleID, Size);
        p += NameLen + 1;
        //TypeKind
        BYTE TypeKind = *p; p++;
        fprintf(fOut, " (KIND=%c)\n", TypeKind);
        //VMCnt
        WORD VMCnt = *((WORD*)p); p += sizeof(WORD);
        if (VMCnt) fprintf(fOut, "  VMCNT=%d\n", VMCnt);
        //Decl
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "  DECL=%s\n", p);
        p += NameLen + 1;
        //DumpTotal
        DumpTotal = *((DWORD*)p); p += sizeof(DWORD);
        //DumpSz
        DumpSize = *((DWORD*)p); p += sizeof(DWORD);
        //FixupNum
        FixupNum = *((DWORD*)p); p += sizeof(DWORD);
        //Dump
        if (DumpSize)
        {
            fprintf(fOut, "  DUMP=");
            for (int m = 0; m < DumpSize; m++)
            {
                if (m) fprintf(fOut, " ");
                fprintf(fOut, "%02X", *p);
                p++;
            }
            fprintf(fOut, "\n");
            fprintf(fOut, "  RELOCS=");
            for (int m = 0; m < DumpSize; m++)
            {
                if (m) fprintf(fOut, " ");
                fprintf(fOut, "%02X", *p);
                p++;
            }
            fprintf(fOut, "\n");
            //Fixups
            for (int m = 0; m < FixupNum; m++)
            {
                if (!m) fprintf(fOut, "  FIXUPS\n");
                //Type
                FixupType = *p; p++;
                fprintf(fOut, "  %c", FixupType);
                //Offset
                FixupOffset = *((DWORD*)p); p += sizeof(DWORD);
                fprintf(fOut, " %lX", FixupOffset);
                //Name
                NameLen = *((WORD*)p); p += sizeof(WORD);
                fprintf(fOut, " %s", p);
                p += NameLen + 1;
                fprintf(fOut, "\n");
            }
        }
        //FieldsTotal
        DWORD FieldsTotal = *((DWORD*)p); p += sizeof(DWORD);
        //FieldNum
        WORD FieldsNum = *((WORD*)p); p += sizeof(WORD);
        //Fields
        for (int m = 0; m < FieldsNum; m++)
        {
            if (!m) fprintf(fOut, "  FIELDS\n");
            fprintf(fOut, "  ");
            //Scope
            BYTE Scope = *p; p++;
            switch (Scope)
            {
            case 9:
                fprintf(fOut, "private");
                break;
            case 10:
                fprintf(fOut, "protected");
                break;
            case 11:
                fprintf(fOut, "public");
                break;
            case 12:
                fprintf(fOut, "published");
                break;
            default:
                fprintf(fOut, "???");
                break;
            }
            //Ndx
            int Ndx = *((int*)p); p += sizeof(int);
            //Case
            int Case = *((int*)p); p += sizeof(int);
            //Name
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, " %s", p);
            p += NameLen + 1;
            //TypeDef
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, " %s", p);
            p += NameLen + 1;
            fprintf(fOut, " Ofs=%d", Ndx);
            if (Case != -1) fprintf(fOut, " Case=%d", Case);
            fprintf(fOut, "\n");
        }
        //PropsTotal
        DWORD PropsTotal = *((DWORD*)p); p += sizeof(DWORD);
        //PropsNum
        WORD PropsNum = *((WORD*)p); p += sizeof(WORD);
        //Properties
        for (int m = 0; m < PropsNum; m++)
        {
            if (!m) fprintf(fOut, "  PROPS\n");
            fprintf(fOut, "  ");
            //Scope
            BYTE Scope = *p; p++;
            switch (Scope)
            {
            case 9:
                fprintf(fOut, "private");
                break;
            case 10:
                fprintf(fOut, "protected");
                break;
            case 11:
                fprintf(fOut, "public");
                break;
            case 12:
                fprintf(fOut, "published");
                break;
            default:
                fprintf(fOut, "???");
                break;
            }
            int Index = *((int*)p); p += sizeof(int);
            //DispID
            int DispID = *((int*)p); p += sizeof(int);
            //Name
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, " %s", p);
            p += NameLen + 1;
            //TypeDef
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, " %s", p);
            p += NameLen + 1;
            //ReadName
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, " %s", p);
            p += NameLen + 1;
            //WriteName
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, " %s", p);
            p += NameLen + 1;
            //StoredName
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, " %s", p);
            p += NameLen + 1;
            if (Index != -1)
            {
                switch (Index & 6)
                {
                case 2:
                    fprintf(fOut, " readonly");
                    break;
                case 4:
                    fprintf(fOut, " writeonly");
                    break;
                }
            }
            fprintf(fOut, " dispid $%lX", DispID);
            fprintf(fOut, "\n");
        }
        //MethodsTotal
        DWORD MethodsTotal = *((DWORD*)p); p += sizeof(DWORD);
        //MethodsNum
        WORD MethodsNum = *((WORD*)p); p += sizeof(WORD);
        //Methods
        for (int m = 0; m < MethodsNum; m++)
        {
            if (!m) fprintf(fOut, "  METHODS\n");
            fprintf(fOut, "  ");
            //Scope
            BYTE Scope = *p; p++;
            //MethodKind
            BYTE MethodKind = *p; p++;
            fprintf(fOut, "%c ", MethodKind);
            switch (Scope)
            {
            case 9:
                fprintf(fOut, "private");
                break;
            case 10:
                fprintf(fOut, "protected");
                break;
            case 11:
                fprintf(fOut, "public");
                break;
            case 12:
                fprintf(fOut, "published");
                break;
            default:
                fprintf(fOut, "???");
                break;
            }
            //Prototype
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, " %s", p);
            p += NameLen + 1;
            fprintf(fOut, "\n");
        }
    }
    delete[] TmpBuf;

    //Test Var Section
    TmpBuf = new BYTE[MaxVarDataSize];
    for (int n = 0; n < VarCount; n++)
    {
        ID = VarOffsets[n].NamID;
        fseek(fIn, VarOffsets[ID].Offset, SEEK_SET);
        fread(TmpBuf, 1, VarOffsets[ID].Size, fIn);
        p = TmpBuf;
        //ModuleID
        ModuleID = *((WORD*)p); p += sizeof(WORD);
        //Name
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "VAR %s (ID%d)", p, ModuleID);
        p += NameLen + 1;
        //Var Type
        BYTE VarType = *p; p++;
        fprintf(fOut, "(TYPE=%c)\n", VarType);
        //TypeDef
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "  TYPEDEF=%s\n", p);
        p += NameLen + 1;
        //AbsName
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "  ABSNAME=%s\n", p);
        p += NameLen + 1;
    }
    delete[] TmpBuf;

    //Test ResStr Section
    TmpBuf = new BYTE[MaxResStrDataSize];
    for (int n = 0; n < ResStrCount; n++)
    {
        ID = ResStrOffsets[n].NamID;
        fseek(fIn, ResStrOffsets[ID].Offset, SEEK_SET);
        fread(TmpBuf, 1, ResStrOffsets[ID].Size, fIn);
        p = TmpBuf;
        //ModuleID
        ModuleID = *((WORD*)p); p += sizeof(WORD);
        //Name
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "RESSTR %s (ID%d)\n", p, ModuleID);
        p += NameLen + 1;
        //TypeDef
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "  TYPEDEF=%s\n", p);
        p += NameLen + 1;
        //Context
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "  CONTEXT=%s\n", p);
    }
    delete[] TmpBuf;

    //Test Proc Section
    TmpBuf = new BYTE[MaxProcDataSize];
    for (int n = 0; n < ProcCount; n++)
    {
        ID = ProcOffsets[n].NamID;
        fseek(fIn, ProcOffsets[ID].Offset, SEEK_SET);
        fread(TmpBuf, 1, ProcOffsets[ID].Size, fIn);
        p = TmpBuf;
        //ModuleID
        ModuleID = *((WORD*)p); p += sizeof(WORD);
        //Name
        NameLen = *((WORD*)p); p += sizeof(WORD);
        fprintf(fOut, "PROC %s (ID%d)", p, ModuleID);
        p += NameLen + 1;
        //Embedded
        BYTE Embedded = *p; p++;
        //DumpType
        BYTE DumpType = *p; p++;
        //MethodKind
        BYTE MethodKind = *p; p++;
        fprintf(fOut, " (%c)", MethodKind);
        //CallKind
        BYTE CallKind = *p; p++;
        fprintf(fOut, " %s\n", CallKindName[CallKind].c_str());
        //VProc
        int VProc = *((int*)p); p += sizeof(int);
        //TypeDef
        NameLen = *((WORD*)p); p += sizeof(WORD);
        if (NameLen) fprintf(fOut, "  PROCTYPEDEF=%s\n", p);
        p += NameLen + 1;
        //DumpTotal
        DumpTotal = *((DWORD*)p); p += sizeof(DWORD);
        //DumpSz
        DumpSize = *((DWORD*)p); p += sizeof(DWORD);
        //FixupNum
        FixupNum = *((DWORD*)p); p += sizeof(DWORD);
        //Dump
        if (DumpSize)
        {
            if (DumpType == 'C')
                fprintf(fOut, "  CODE=");
            else
                fprintf(fOut, "  DATA=");

            for (int m = 0; m < DumpSize; m++)
            {
                if (m) fprintf(fOut, " ");
                fprintf(fOut, "%02X", *p);
                p++;
            }
            fprintf(fOut, "\n");
            fprintf(fOut, "  RELOCS=");
            for (int m = 0; m < DumpSize; m++)
            {
                if (m) fprintf(fOut, " ");
                fprintf(fOut, "%02X", *p);
                p++;
            }
            fprintf(fOut, "\n");
            //Fixups
            for (int m = 0; m < FixupNum; m++)
            {
                if (!m) fprintf(fOut, "  FIXUPS\n");
                //Type
                FixupType = *p; p++;
                fprintf(fOut, "  FT%c", FixupType);
                //Offset
                FixupOffset = *((DWORD*)p); p += sizeof(DWORD);
                fprintf(fOut, " %lX", FixupOffset);
                //Name
                NameLen = *((WORD*)p); p += sizeof(WORD);
                fprintf(fOut, " %s", p);
                p += NameLen + 1;
                fprintf(fOut, "\n");
            }
        }
        //ArgsTotal
        DWORD ArgsTotal = *((DWORD*)p); p += sizeof(DWORD);
        //ArgsNum
        WORD ArgsNum = *((WORD*)p); p += sizeof(WORD);
        //Args
        for (int m = 0; m < ArgsNum; m++)
        {
            if (!m) fprintf(fOut, "  ARGS\n");
            fprintf(fOut, "  ");
            //Tag
            BYTE Tag = *p; p++;
            switch (Tag)
            {
            case 33:
                fprintf(fOut, "val ");
                break;
            case 34:
                fprintf(fOut, "var ");
                break;
            default:
                fprintf(fOut, "??? ");
                break;
            }
            //LocFlags
            int LocFlags = *((int*)p); p += sizeof(int);
            if ((LocFlags & 8) != 0 && Tag != 0x2C)
            	fprintf(fOut, "{r}");
            else
            	fprintf(fOut, "{s}");
            //Ndx
            int Ndx = *((int*)p); p += sizeof(int);
            fprintf(fOut, " Ndx=%d", Ndx);
            //Name
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, " %s", p);
            p += NameLen + 1;
            //TypeDef
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, ":%s", p);
            p += NameLen + 1;
            fprintf(fOut, "\n");
        }
        /*
        //LocalsTotal
        DWORD LocalsTotal = *((DWORD*)p); p += sizeof(DWORD);
        //LocalsNum
        WORD LocalsNum = *((WORD*)p); p += sizeof(WORD);
        //Locals
        for (int m = 0; m < LocalsNum; m++)
        {
            if (!m) fprintf(fOut, "  LOCALS\n");
            fprintf(fOut, "  ");
            //Tag
            BYTE Tag = *p; p++;
            switch (Tag)
            {
            case 32:
                fprintf(fOut, "local ");
                break;
            case 33:
                fprintf(fOut, "Lval ");
                break;
            case 34:
                fprintf(fOut, "Lvar ");
                break;
            case 35:
                fprintf(fOut, "result ");
                break;
            case 36:
                fprintf(fOut, "local absolute ");
                break;
            default:
                fprintf(fOut, "??? ");
                break;
            }
            //LocFlags
            int LocFlags = *((int*)p); p += sizeof(int);
            //Ndx
            int Ndx = *((int*)p); p += sizeof(int);
            //Name
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, " %s", p);
            p += NameLen + 1;
            //TypeDef
            NameLen = *((WORD*)p); p += sizeof(WORD);
            fprintf(fOut, ":%s", p);
            p += NameLen + 1;
            //AbsName
            NameLen = *((WORD*)p); p += sizeof(WORD);
            if (NameLen) fprintf(fOut, " ABSNAME=%s", p);
            p += NameLen + 1;
            fprintf(fOut, "\n");
        }
        */
    }
    delete[] TmpBuf;

    delete[] ModuleOffsets;
    delete[] ConstOffsets;
    delete[] TypeOffsets;
    delete[] VarOffsets;
    delete[] ResStrOffsets;
    delete[] ProcOffsets;

    fclose(fIn);
    return 0;
}
//---------------------------------------------------------------------------
