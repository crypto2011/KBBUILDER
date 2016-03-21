{$A+,B-,C+,D+,E-,F-,G+,H+,I+,J+,K-,L+,M-,N+,O+,P+,Q-,R-,S-,T-,U-,V+,W-,X+,Y+,Z1}
{$APPTYPE CONSOLE}
program dcu32int;
(*
The main module of the DCU32INT utility by Alexei Hmelnov.
----------------------------------------------------------------------------
E-Mail: alex@icc.ru
http://hmelnov.icc.ru/DCU/
----------------------------------------------------------------------------

See the file "readme.txt" for more details.

------------------------------------------------------------------------
                             IMPORTANT NOTE:
This software is provided 'as-is', without any expressed or implied warranty.
In no event will the author be held liable for any damages arising from the
use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented, you must not
   claim that you wrote the original software.
2. Altered source versions must be plainly marked as such, and must not
   be misrepresented as being the original software.
3. This notice may not be removed or altered from any source
   distribution.
*)
//  {$IFNDEF LINUX}Windows,{$ELSE}LinuxFix,{$ENDIF}

uses
  SysUtils,
  //crypto_
  Classes,
  //_crypto
  DCU32 in 'DCU32.pas',
  DCUTbl in 'DCUTbl.pas',
  DCU_In in 'DCU_In.pas',
  DCU_Out in 'DCU_Out.pas',
  FixUp in 'FixUp.pas',
  DCURecs in 'DCURecs.pas',
  DasmDefs in 'DasmDefs.pas',
  DasmCF in 'DasmCF.pas',
  DCP in 'DCP.pas',
  DasmX86 in 'DasmX86.pas',
  DasmMSIL in 'DasmMSIL.pas';

{$R *.res}

procedure WriteUsage;
begin
  Writeln(
  'Usage:'#13#10+
  '  DCU32INT <Source file> <Flags> [<Destination file>]'#13#10+
  'Destination file may contain * to be replaced by unit name or name and extension'#13#10+
  'Destination file = "-" => write to stdout.'#13#10+
 {$IFNDEF LINUX}
  'Flags (start with "/" or "-"):'#13#10+
 {$ELSE}
  'Flags (start with "-"):'#13#10+
 {$ENDIF}
  ' -S<show flag>* - Show flags (-S - show all), default: (+) - on, (-) - off'#13#10+
  '    A(-) - show Address table'#13#10+
  '    C(-) - don''t resolve Constant values'#13#10+
  '    D(-) - show Data block'#13#10+
  '    d(-) - show dot types'#13#10+
  '    F(-) - show Fixups'#13#10+
  '    H(+) - show Heuristic strings'#13#10+
  '    I(+) - show Imported names'#13#10+
  '    L(-) - show table of Local variables'#13#10+
  '    M(-) - don''t resolve class Methods'#13#10+
  '    O(-) - show file Offsets'#13#10+
  '    S(-) - show Self arguments of methods and 2nd call flags of `structors'#13#10+
  '    T(-) - show Type table'#13#10+
  '    U(-) - show Units of imported names'#13#10+
  '    V(-) - show auxiliary Values'#13#10+
  '    v(-) - show VMT'#13#10+
  ' -O<option>* - code generation options, default: (+) - on, (-) - off'#13#10+
  '    V(-) - typed constants as variables'#13#10+
  ' -I - interface part only'#13#10+
  ' -U<paths> - Unit directories, * means autodetect by unit version'#13#10+
  ' -P<paths> - Pascal source directories (just "-P" means: "seek for *.pas in'#13#10+
  '    the unit directory"). Without this parameter src lines won''t be reported'#13#10+
  ' -R<Alias>=<unit>[;<Alias>=<unit>]* - set unit aliases'#13#10+
  ' -N<Prefix> - No Name Prefix ("%" - Scope char)'#13#10+
  ' -F<FMT> - output format (T - text (default), H-HTML)'#13#10+
  ' -D<Prefix> - Dot Name Prefix ("%" - Scope char)'#13#10+
  ' -Q<Query flag> - Query additional information.'#13#10+
  '    F(-) - class fields'#13#10+
  '    V(-) - class virtual methods'#13#10+
  ' -A<Mode> - disAssembler mode'#13#10+
  '    S(+) - simple Sequential (all memory is a sequence of ops)'#13#10+
  '    C(-) - control flow'#13#10+
  //crypto_
  ' -K<dcu_list> - build KB file for IDR (-K* - process all units), use with -U flag'#13#10
  //_crypto
  );
end ;

const
  DCUName: String = '';
  FNRes: String = '';

{ Queries: }
type
  TQueryFlag = (qfFields,qfVMT);
  TQueryFlags = set of TQueryFlag;

const
  qfAll = [Low(TQueryFlag)..High(TQueryFlag)];

var
  Queries: TQueryFlags=[];

function ProcessParms: boolean;
var
  i,j: integer;
  PS: String;
  Ch: Char;
begin
  Result := false;
  for i:=1 to ParamCount do begin
    PS := ParamStr(i);
    if (Length(PS)>1)and({$IFNDEF LINUX}(PS[1]='/')or{$ENDIF}(PS[1]='-')) then begin
      Ch := UpCase(PS[2]);
      case Ch of
        'H','?': begin
          WriteUsage;
          Exit;
         end ;
        'S': begin
          if Length(PS)=2 then
            SetShowAll
          else begin
            for j:=3 to Length(PS) do begin
              Ch := {UpCase(}PS[j]{)};
              case Ch of
                'A': ShowAddrTbl := true;
                'C': ResolveConsts := false;
                'D': ShowDataBlock := true;
                'd': ShowDotTypes := true;
                'F': ShowFixupTbl := true;
                'H': ShowHeuristicRefs := false;
                'I': ShowImpNames := false;
                'L': ShowLocVarTbl := true;
                'M': ResolveMethods := false;
                'O': ShowFileOffsets := true;
                'S': ShowSelf := true;
                'T': ShowTypeTbl := true;
                'U': ShowImpNamesUnits := true;
                'V': ShowAuxValues := true;
                'v': ShowVMT := true;
              else
                Writeln('Unknown show flag: "',Ch,'"');
                Exit;
              end ;
            end ;
          end ;
        end ;
        'Q': begin
          if Length(PS)=2 then
            Queries := qfAll
          else begin
            Queries := [];
            for j:=3 to Length(PS) do begin
              Ch := {UpCase(}PS[j]{)};
              case Ch of
               'F': Include(Queries,qfFields);
               'V': Include(Queries,qfVMT);
              else
                Writeln('Unknown query flag: "',Ch,'"');
                Exit;
              end ;
            end ;
          end ;
        end ;
        'O':
          for j:=3 to Length(PS) do begin
            Ch := {UpCase(}PS[j]{)};
            case Ch of
              'V': GenVarCAsVars := true;
            else
              Writeln('Unknown code generation option: "',Ch,'"');
              Exit;
            end ;
          end ;
        'I': InterfaceOnly := true;
        'U': begin
          Delete(PS,1,2);
          DCUPath := PS;
        end ;
        'R': begin
          Delete(PS,1,2);
          SetUnitAliases(PS);
        end ;
        'P': begin
          Delete(PS,1,2);
          PASPath := PS;
        end ;
        'N': begin
          Delete(PS,1,2);
          NoNamePrefix := PS;
        end ;
        'D': begin
          Delete(PS,1,2);
          DotNamePrefix := PS;
        end ;
        'A': begin
           if Length(PS)=2 then
             Ch := 'C'
           else
             Ch := UpCase(PS[3]);
           case Ch of
            'S': DasmMode := dasmSeq;
            'C': DasmMode := dasmCtlFlow;
           else
             Writeln('Unknown disassembler mode: "',Ch,'"');
             Exit;
           end ;
        end ;
        'F': begin
          if (Length(PS)>2)and(UpCase(PS[3])='H') then
            OutFmt := ofmtHTM;
        end ;
        //crypto_
        'K':
        begin
          Delete(PS, 1, 2);
          KBUnitsList := PS;
          SetShowAll;
          //FNRes := '-';
          Result := True;
          Exit;
        end;
        //_crypto
      else
        Writeln('Unknown flag: "',Ch,'"');
        Exit;
      end ;
      Continue;
    end ;
    if DCUName='' then
      DCUName := PS
    else if FNRes='' then
      FNRes := PS
    else
      Exit;
  end ;
  Result := DCUName<>'';
end ;

procedure QueryUnit(U: TUnit; Queries: TQueryFlags);
{ Output information to simplify disassembly analysis}

  function ShowFields(U1: TUnit; Hdr: String; F: TNameDecl): Boolean;
  begin
    Result := false;
    while F<>Nil do begin
      if (F is TLocalDecl)and(F.GetTag=arFld) then begin
        if not Result then begin
          Result := true;
          if Hdr<>'' then begin
            Writer.NLOfs := 4;
            NL;
            PutS(Hdr);
          end ;
          Writer.NLOfs := 6;
        end ;
        NL;
        PutSFmt('@%d=$%0:x ',[TLocalDecl(F).Ndx]);
        PutS(F.Name^.GetStr);
        PutS(': ');
        U1.ShowTypeDef(TLocalDecl(F).hDT,Nil);
      end ;
      F := TNameDecl(F.Next);
    end ;
  end ;

  function ShowParentFields(U1: TUnit; hParent: TNDX): Boolean;
  var
    TD: TTypeDef;
    U2: TUnit;
  begin
    Result := false;
    TD := U1.GetGlobalTypeDef(hParent,U2);
    if TD=Nil then
      Exit;
    if not(TD is TRecBaseDef) then
      Exit;
    if TD is TOOTypeDef then begin
      if ShowParentFields(U2,TOOTypeDef(TD).hParent) then
        Result := true;
    end ;
    if ShowFields(U2,TD.Name^.GetStr,TRecBaseDef(TD).Fields) then
      Result := true;
  end ;

  function ShowMethods(U1: TUnit; Hdr: String; F: TNameDecl): Boolean;
  begin
    Result := false;
    while F<>Nil do begin
      if (F is TMethodDecl)and(TMethodDecl(F).LocFlags and lfMethodKind in [lfVirtual])
        and(TMethodDecl(F).LocFlags and lfOverride=0{Don't show it again})
      then begin
        if not Result then begin
          Result := true;
          if Hdr<>'' then begin
            Writer.NLOfs := 4;
            NL;
            PutS(Hdr);
          end ;
          Writer.NLOfs := 6;
        end ;
        NL;
        PutSFmt('[%d=$%0:x] ',[TLocalDecl(F).hDT*4]);
        PutS(F.Name^.GetStr);
      end ;
      F := TNameDecl(F.Next);
    end ;
  end ;

  function ShowParentMethods(U1: TUnit; hParent: TNDX): Boolean;
  var
    TD: TTypeDef;
    U2: TUnit;
  begin
    Result := false;
    TD := U1.GetGlobalTypeDef(hParent,U2);
    if TD=Nil then
      Exit;
    if not(TD is TOOTypeDef) then
      Exit;
    if ShowParentMethods(U2,TOOTypeDef(TD).hParent) then
      Result := true;
    if ShowMethods(U2,TD.Name^.GetStr,TOOTypeDef(TD).Fields) then
      Result := true;
  end ;

var
  i: Integer;
  TD: TTypeDef;
  U1: TUnit;
  Hdr: String;
begin
  NL;
  PutKW('queries');
  Writer.NLOfs := 2;
  NL;
  for i:=1 to U.TypeCount do begin
    TD := U.GetGlobalTypeDef(i,U1);
    if TD=Nil then
      Continue;
    if (TD is TRecBaseDef)and((TD is TOOTypeDef)or(TD is TRecDef)) then begin
      if U1<>U then begin
        PutS(U1.UnitName);
        PutCh('.');
      end ;
      PutS(TD.Name^.GetStr);
      if qfFields in Queries then begin
        Hdr := '<FIELDS>';
        if TD is TOOTypeDef then
          {if} ShowParentFields(U1,TOOTypeDef(TD).hParent) {then
            Hdr := '<FIELDS>'};
        ShowFields(U1,Hdr,TRecBaseDef(TD).Fields);
      end ;
      if (qfVMT in Queries)and(TD is TOOTypeDef) then begin
        ShowParentMethods(U1,TOOTypeDef(TD).hParent);
        ShowMethods(U1,'<METHODS>',TOOTypeDef(TD).Fields);
      end ;
      {
  VMCnt: TNDX;//number of virtual methods
      end ;}
      Writer.NLOfs := 2;
      NL;
    end ;
  end ;
end ;


function ReplaceStar(FNRes,FN: String): String;
var
  CP: PChar;
begin
  CP := StrScan(PChar(FNRes),'*');
  if CP=Nil then begin
    Result := FNRes;
    Exit;
  end ;
  if StrScan(CP+1,'*')<>Nil then
    raise Exception.Create('2nd "*" is not allowed');
  FN := ExtractFilename(FN);
  if (CP+1)^=#0 then begin
    Result := Copy(FNRes,1,CP-PChar(FNRes))+ChangeFileExt(FN,'.int');
    Exit;
  end;
  Result := Copy(FNRes,1,CP-PChar(FNRes))+ChangeFileExt(FN,'')+Copy(FNRes,CP-PChar(FNRes)+2,MaxInt);
end ;

function ProcessFile(FN: String): integer {ErrorLevel};
var
  U: TUnit;
  NS: String;
  ExcS: String;
  OutRedir: boolean;
  CP: PChar;
  //crypto_
  //Writer: TBaseWriter;
  //_crypto
begin
  Result := 0;
  OutRedir := false;
  if FNRes='-' then
    FNRes := ''
  else begin
    Writeln{(StdErr)};
    Writeln('File: "',FN,'"');
    NS := ExtractFileName(FN);
    CP := StrScan(PChar(NS),PkgSep);
    if CP<>Nil then
      NS := StrPas(CP+1);
    //if FNRes='' then
    //  FNRes := ExtractFilePath(FN)+ChangeFileExt(NS,DefaultExt[OutFmt])
    //else
    //  FNRes := ReplaceStar(FNRes,FN);
    FNRes := ExtractFilePath(FN) + '$$$.int';
    Writeln('Result: "',FNRes,'"');
//    CloseFile(Output);
    Flush(Output);
    OutRedir := true;
  end ;
  Writer := Nil;
  {AssignFile(FRes,FNRes);
  TTextRec(FRes).Mode := fmClosed;}
  try
    try
      //Rewrite(FRes); //Test whether the FNRes is a correct file name
      try
        Writer := InitOut(FNRes);
        FN := ExpandFileName(FN);
        //crypto_
        GetMem(ModuleInfo, sizeof(MODULE_INFO));
        FillChar(ModuleInfo^, sizeof(MODULE_INFO), 0);
        ModuleInfo^.ModuleID := ModuleList.Count;
        ModuleInfo^.UsesList := TStringList.Create;
        ModuleID := ModuleInfo^.ModuleID;
        ModuleInfo^.Filename := NS;
        ModuleList.Add(ModuleInfo);
        //_crypto
        U := Nil;
        try
          U := GetDCUByName(FN,'',0,false{Will be ignored 'cause verRq=0},
            dcuplWin32{Will be ignored 'cause verRq=0},0){TUnit.Create(FN)};
        finally
          if U=Nil then
            U := MainUnit;
          if U<>Nil then begin
            //crypto_
            ModuleInfo^.Name := U.UnitName;
            //_crypto
            U.Show;
            if Queries<>[] then
              QueryUnit(U,Queries);
          end ;
        end ;
      finally
        FreeDCU;
      end ;
    except
      on E: Exception do begin
        Result := 1;
        ExcS := Format('!!!%s: "%s"',[E.ClassName,E.Message]);
        if Writer<>Nil then
          ReportExc(ExcS);
        {if TTextRec(FRes).Mode<>fmClosed then begin
          Writeln(FRes);
          Writeln(FRes,ExcS);
          Flush(FRes);
        end ;}
        if OutRedir then
          Writeln(ExcS);
      end ;
    end ;
  finally
    if OutRedir then begin
      Writeln(Format('Total %d lines generated.',[Writer.OutLineNum]));
      Close(Output);
    end ;
    Writer.Free;
    {if TTextRec(FRes).Mode<>fmClosed then begin
      DoneOut;
      Close(FRes);
    end ;}
  end ;
end ;

//crypto_
function CompareModulesByName(Item1:Pointer; Item2:Pointer):Integer;
begin
  Result := CompareText(PMODULE_INFO(Item1)^.Name, PMODULE_INFO(Item2)^.Name);
end;

function CompareModulesByID(Item1:Pointer; Item2:Pointer):Integer;
begin
  if (PMODULE_INFO(Item1)^.ModuleID > PMODULE_INFO(Item2)^.ModuleID) then
    Result := 1
  else if (PMODULE_INFO(Item1)^.ModuleID < PMODULE_INFO(Item2)^.ModuleID) then
    Result := -1
  else
    Result := 0;
end;

function CompareConstsByName(Item1:Pointer; Item2:Pointer):Integer;
begin
  Result := CompareText(PCONST_INFO(Item1)^.Name, PCONST_INFO(Item2)^.Name);
  if (Result <> 0) then
    Exit;
  if (PCONST_INFO(Item1)^.ModuleID > PCONST_INFO(Item2)^.ModuleID) then
    Result := 1
  else if (PCONST_INFO(Item1)^.ModuleID < PCONST_INFO(Item2)^.ModuleID) then
    Result := -1
  else
    Result := 0;
end;

function CompareConstsByID(Item1:Pointer; Item2:Pointer):Integer;
begin
  if (PCONST_INFO(Item1)^.ModuleID > PCONST_INFO(Item2)^.ModuleID) then
    Result := 1
  else if (PCONST_INFO(Item1)^.ModuleID < PCONST_INFO(Item2)^.ModuleID) then
    Result := -1
  else
    Result := CompareText(PCONST_INFO(Item1)^.Name, PCONST_INFO(Item2)^.Name);
end;

function CompareTypesByName(Item1:Pointer; Item2:Pointer):Integer;
begin
  Result := CompareText(PTYPE_INFO(Item1)^.Name, PTYPE_INFO(Item2)^.Name);
  if (Result <> 0) then
    Exit;
  if (PTYPE_INFO(Item1)^.ModuleID > PTYPE_INFO(Item2)^.ModuleID) then
    Result := 1
  else if (PTYPE_INFO(Item1)^.ModuleID < PTYPE_INFO(Item2)^.ModuleID) then
    Result := -1
  else
    Result := 0;
end;

function CompareTypesByID(Item1:Pointer; Item2:Pointer):Integer;
begin
  if (PTYPE_INFO(Item1)^.ModuleID > PTYPE_INFO(Item2)^.ModuleID) then
    Result := 1
  else if (PTYPE_INFO(Item1)^.ModuleID < PTYPE_INFO(Item2)^.ModuleID) then
    Result := -1
  else
    Result := CompareText(PTYPE_INFO(Item1)^.Name, PTYPE_INFO(Item2)^.Name);
end;

function CompareVarsByName(Item1:Pointer; Item2:Pointer):Integer;
begin
  Result := CompareText(PVAR_INFO(Item1)^.Name, PVAR_INFO(Item2)^.Name);
  if (Result <> 0) then
    Exit;
  if (PVAR_INFO(Item1)^.ModuleID > PVAR_INFO(Item2)^.ModuleID) then
    Result := 1
  else if (PVAR_INFO(Item1)^.ModuleID < PVAR_INFO(Item2)^.ModuleID) then
    Result := -1
  else
    Result := 0;
end;

function CompareVarsByID(Item1:Pointer; Item2:Pointer):Integer;
begin
  if (PVAR_INFO(Item1)^.ModuleID > PVAR_INFO(Item2)^.ModuleID) then
    Result := 1
  else if (PVAR_INFO(Item1)^.ModuleID < PVAR_INFO(Item2)^.ModuleID) then
    Result := -1
  else
    Result := CompareText(PVAR_INFO(Item1)^.Name, PVAR_INFO(Item2)^.Name);
end;

function CompareResStrsByName(Item1:Pointer; Item2:Pointer):Integer;
begin
  Result := CompareText(PRESSTR_INFO(Item1)^.Name, PRESSTR_INFO(Item2)^.Name);
  if (Result <> 0) then
    Exit;
  if (PRESSTR_INFO(Item1)^.ModuleID > PRESSTR_INFO(Item2)^.ModuleID) then
    Result := 1
  else if (PRESSTR_INFO(Item1)^.ModuleID < PRESSTR_INFO(Item2)^.ModuleID) then
    Result := -1
  else
    Result := 0;
end;

function CompareResStrsByID(Item1:Pointer; Item2:Pointer):Integer;
begin
  if (PRESSTR_INFO(Item1)^.ModuleID > PRESSTR_INFO(Item2)^.ModuleID) then
    Result := 1
  else if (PRESSTR_INFO(Item1)^.ModuleID < PRESSTR_INFO(Item2)^.ModuleID) then
    Result := -1
  else
    Result := CompareText(PRESSTR_INFO(Item1)^.Name, PRESSTR_INFO(Item2)^.Name);
end;

function CompareProcsByName(Item1:Pointer; Item2:Pointer):Integer;
begin
  Result := CompareText(PPROCDECL_INFO(Item1)^.Name, PPROCDECL_INFO(Item2)^.Name);
  if (Result <> 0) then
    Exit;
  if (PPROCDECL_INFO(Item1)^.ModuleID > PPROCDECL_INFO(Item2)^.ModuleID) then
    Result := 1
  else if (PPROCDECL_INFO(Item1)^.ModuleID < PPROCDECL_INFO(Item2)^.ModuleID) then
    Result := -1
  else
    Result := 0;
end;

function CompareProcsByID(Item1:Pointer; Item2:Pointer):Integer;
begin
  if (PPROCDECL_INFO(Item1)^.ModuleID > PPROCDECL_INFO(Item2)^.ModuleID) then
    Result := 1
  else if (PPROCDECL_INFO(Item1)^.ModuleID < PPROCDECL_INFO(Item2)^.ModuleID) then
    Result := -1
  else
    Result := CompareText(PPROCDECL_INFO(Item1)^.Name, PPROCDECL_INFO(Item2)^.Name);
end;

function WriteString(fDst:TFileStream; str:string):Integer;
var
  Bytes:Integer;
  ZeroB:Byte;
  NameLength:WORD;
begin
  Bytes := 0;
  ZeroB := 0;

  NameLength := Length(str);
  if (fDst <> Nil) then
  begin
    fDst.Write(NameLength, sizeof(NameLength));
    if (NameLength > 0) then
    begin
        fDst.Write(Pointer(str)^, NameLength);
    end;
    fDst.Write(ZeroB, 1);
  end;
  Result := sizeof(NameLength) + NameLength + 1;
end;

function WriteDump(fSrc:TFileStream; fDst:TFileStream; SrcOffset:Cardinal; Bytes:Cardinal):Integer;
var
  b:BYTE;
  m:Integer;
begin
  fSrc.Seek(SrcOffset, soFromBeginning);
  for m := 1 to Bytes do
  begin
    fSrc.Read(b, 1);
    fDst.Write(b, 1);
  end;
  Result := Bytes;
end;

function WriteRelocs(fDst:TFileStream; FixupList:TList; Bytes:Cardinal; Name:String):Integer;
var
  n, m:Integer;
  Byte00:BYTE;
  ByteFF:Cardinal;
  ByteNo:Integer;
  finfo:PFIXUP_INFO;
begin
  Byte00 := 0;
  ByteFF := $FFFFFFFF;
  ByteNo := 0;
  for n := 0 to FixupList.Count - 1 do
  begin
    finfo := PFIXUP_INFO(FixupList.Items[n]);
    //Если информация о фиксапе занимает больше места, чем нужно, не записываем ее
    if (finfo^.Ofs + 4 > Bytes) then
      continue;
    if (finfo^.Ofs < ByteNo) then
      continue;
    //Свободное от релоков место заполняем 0
    for m := ByteNo to finfo^.Ofs - 1 do
    begin
      fDst.Write(Byte00, 1);
      Inc(ByteNo);
    end;
    //Сам релок заполняем 4-мя байтами 0xFF
    fDst.Write(ByteFF, 4); Inc(ByteNo, 4);
  end;
  //Оставшиеся байты заполняем 0
  for m := ByteNo to Bytes - 1 do
  begin
    fDst.Write(Byte00, 1);
  end;
  Result := Bytes;
end;

function WriteFixups(fDst:TFileStream; FixupList:TList):Integer;
var
  m:Integer;
  Bytes:Integer;
  finfo:PFIXUP_INFO;
begin
  Bytes := 0;
  for m := 0 to FixupList.Count -1 do
  begin
    finfo := PFIXUP_INFO(FixupList.Items[m]);
    //write Type
    if (fDst <> Nil) then
      fDst.Write(finfo^.FType, sizeof(finfo^.FType));
    Inc(Bytes, sizeof(finfo^.FType));
    //write Offset
    if (fDst <> Nil) then
      fDst.Write(finfo^.Ofs, sizeof(finfo^.Ofs));
    Inc(Bytes, sizeof(finfo^.Ofs));
    //write Name
    Inc(Bytes, WriteString(fDst, finfo^.Name));
  end;
  Result := Bytes;
end;

procedure KBWriteModules(fDst:TFileStream);
var
  found:Boolean;
  mID:WORD;
  n, m, mm:Integer;
  UsesNum:WORD;
  DataSize:Cardinal;
  ModuleInfo, modInfo:PMODULE_INFO;
begin
  ModuleCount := ModuleList.Count;
  for n := 0 to ModuleCount - 1 do
  begin
    ModuleInfo := PMODULE_INFO(ModuleList.Items[n]);
    ModuleInfo^.ID := n;
    ModuleInfo^.Offset := CurrOffset;
    DataSize := 0;
    //ModuleID
    fDst.Write(ModuleInfo^.ModuleID, sizeof(WORD));
    Inc(DataSize, sizeof(WORD));
    //Name
    Inc(DataSize, WriteString(fDst, ModuleInfo^.Name));
    //Filename
    Inc(DataSize, WriteString(fDst, ModuleInfo^.Filename));
    //UsesNum
    UsesNum := ModuleInfo.UsesList.Count;
    fDst.Write(UsesNum, sizeof(UsesNum));
    Inc(DataSize, sizeof(UsesNum));
    //Uses
    for m := 0 to UsesNum - 1 do
    begin
      found := False;
      for mm := 0 to ModuleCount - 1 do
      begin
        modInfo := PMODULE_INFO(ModuleList.Items[mm]);
        if SameText(modInfo.Name, ModuleInfo.UsesList.Strings[m]) then
        begin
          fDst.Write(modInfo^.ModuleID, sizeof(WORD));
          found := True;
          Break;
        end;
      end;
      if (found = False) then
      begin
        mID := $FFFF;
        fDst.Write(mID, sizeof(WORD));
      end;
      Inc(DataSize, sizeof(WORD));
    end;
    for m := 0 to UsesNum - 1 do
    begin
      Inc(DataSize, WriteString(fDst, ModuleInfo.UsesList.Strings[m]));
    end;

    ModuleInfo.Size := DataSize;
    if (DataSize > MaxModuleDataSize) then
    begin
      MaxModuleDataSize := DataSize;
    end;
    Inc(CurrOffset, DataSize);
  end;
end;

procedure KBWriteConstants(fDst:TFileStream);
var
  fIn:TFileStream;
  n, m:Integer;
  PrevModId:WORD;
  DataSize:Cardinal;
  DumpTotal:Cardinal;
  FixupNum:Cardinal;
  PrevName:string;
  constInfo:PCONST_INFO;
begin
  fIn := Nil;
  ConstList.Sort(CompareConstsByName);
  PrevModId := $FFFF;
  PrevName := '';

  for n := 0 to ConstList.Count - 1 do
  begin
    constInfo := PCONST_INFO(ConstList.Items[n]);
    constInfo^.Skip := False;
    if (SameText(constInfo^.Name, PrevName)) And (constInfo^.ModuleID = PrevModId) then
    begin
      constInfo^.Skip := True;
      continue;
    end;
    constInfo^.ID := ConstCount;
    constInfo^.Offset := CurrOffset;
    PrevModId := constInfo^.ModuleID;
    PrevName := constInfo^.Name;

    DataSize := 0;
    //ModuleID
    fDst.Write(constInfo^.ModuleID, sizeof(constInfo^.ModuleID));
    Inc(DataSize, sizeof(constInfo^.ModuleID));
    //Name
    Inc(DataSize, WriteString(fDst, constInfo^.Name));
    //Type
    fDst.Write(constInfo^.FType, sizeof(constInfo^.FType));
    Inc(DataSize, sizeof(constInfo^.FType));
    //TypeDef
    Inc(DataSize, WriteString(fDst, constInfo^.TypeDef));
    //Value
    Inc(DataSize, WriteString(fDst, constInfo^.Value));
    //Не будем дампить константы с внутренними именами
    if (Pos('_NV_', constInfo^.Name) = 1) then
      constInfo^.RTTISz := 0;
    //DumpTotal
    DumpTotal := 0;
    if (constInfo^.RTTISz <> 0) then
    begin
      fIn := Nil;
      for m := 0 to ModuleCount - 1 do
      begin
        ModuleInfo := PMODULE_INFO(ModuleList.Items[m]);
        if (ModuleInfo^.ModuleID = constInfo^.ModuleID) then
        begin
          fIn := TFileStream.Create(ModuleInfo^.Filename, fmOpenRead);
          break;
        end;
      end;
      if (fIn <> Nil) then
      begin
        //Dump
        Inc(DumpTotal, constInfo^.RTTISz);
        //Relocs
        Inc(DumpTotal, constInfo^.RTTISz);
        //Fixups
        Inc(DumpTotal, WriteFixups(Nil, constInfo^.Fixups));
      end;
    end;
    Inc(DumpTotal, sizeof(constInfo^.RTTISz)); //DumpSz
    FixupNum := constInfo^.Fixups.Count;
    Inc(DumpTotal, sizeof(FixupNum));          //FixupNum

    fDst.Write(DumpTotal, sizeof(DumpTotal));
    Inc(DataSize, sizeof(DumpTotal));
    //DumpSz
    fDst.Write(constInfo^.RTTISz, sizeof(constInfo^.RTTISz));
    Inc(DataSize, sizeof(constInfo^.RTTISz));
    //FixupNum
    fDst.Write(FixupNum, sizeof(FixupNum));
    Inc(DataSize, sizeof(FixupNum));

    if (constInfo^.RTTISz <> 0) then
    begin
      if (fIn <> Nil) then
      begin
        //Dump
        Inc(DataSize, WriteDump(fIn, fDst, constInfo^.RTTIOfs, constInfo^.RTTISz));
        fIn.Free;
        //Relocs
        Inc(DataSize, WriteRelocs(fDst, constInfo^.Fixups, constInfo^.RTTISz, constInfo^.Name));
        //Fixups
        Inc(DataSize, WriteFixups(fDst, constInfo^.Fixups));
      end;
    end;

    constInfo^.Size := DataSize;
    if (DataSize > MaxConstDataSize) then
      MaxConstDataSize := DataSize;
    Inc(CurrOffset, DataSize);
    Inc(ConstCount);
  end;
end;

procedure KBWriteTypes(fDst:TFileStream);
var
  FieldsNum:WORD;
  PropsNum:WORD;
  MethodsNum:WORD;
  fIn:TFileStream;
  n, m:Integer;
  DataSize:Cardinal;
  DumpTotal:Cardinal;
  PropsTotal:Cardinal;
  MethodsTotal:Cardinal;
  FixupNum:Cardinal;
  FieldsTotal:Cardinal;
  typeInfo:PTYPE_INFO;
  linfo:PLOCALDECL_INFO;
  pinfo:PPROPERTY_INFO;
  minfo:PMETHODDECL_INFO;
begin
  fIn := Nil;
  TypeCount := TypeList.Count;
  for n := 0 to TypeCount -1 do
  begin
    typeInfo := PTYPE_INFO(TypeList.Items[n]);
    typeInfo^.ID := n;
    typeInfo^.Offset := CurrOffset;
    DataSize := 0;
    //Size (NEW_VERSION)
    fDst.Write(typeInfo^.Size, sizeof(typeInfo^.Size));
    Inc(DataSize, sizeof(typeInfo^.Size));
    //ModuleID
    fDst.Write(typeInfo^.ModuleID, sizeof(typeInfo^.ModuleID));
    Inc(DataSize, sizeof(typeInfo^.ModuleID));
    //Name
    Inc(DataSize, WriteString(fDst, typeInfo^.Name));
    //Kind
    fDst.Write(typeInfo^.Kind, sizeof(typeInfo^.Kind));
    Inc(DataSize, sizeof(typeInfo^.Kind));
    //VMCnt
    fDst.Write(typeInfo^.VMCnt, sizeof(typeInfo^.VMCnt));
    Inc(DataSize, sizeof(typeInfo^.VMCnt));
    //Decl
    Inc(DataSize, WriteString(fDst, typeInfo^.Decl));
    //Не будем дампить типы с внутренними именами
    if (Pos('_NT_', typeInfo^.Name) = 1) then
      typeInfo^.RTTISz := 0;
    //DumpTotal
    DumpTotal := 0;
    if (typeInfo^.RTTISz <> 0) then
    begin
      fIn := Nil;
      for m := 0 to ModuleList.Count - 1 do
      begin
        ModuleInfo := PMODULE_INFO(ModuleList.Items[m]);
        if (ModuleInfo^.ModuleID = typeInfo^.ModuleID) then
        begin
          fIn := TFileStream.Create(ModuleInfo^.Filename, fmOpenRead);
          break;
        end;
      end;
      if (fIn <> Nil) then
      begin
        //Dump
        Inc(DumpTotal, typeInfo^.RTTISz);
        //Relocs
        Inc(DumpTotal, typeInfo^.RTTISz);
        //Fixups
        Inc(DumpTotal, WriteFixups(Nil, typeInfo^.Fixups));
      end;
    end;
    Inc(DumpTotal, sizeof(typeInfo^.RTTISz));  //DumpSz
    FixupNum := typeInfo^.Fixups.Count;
    Inc(DumpTotal, sizeof(FixupNum));          //FixupNum

    fDst.Write(DumpTotal, sizeof(DumpTotal));
    Inc(DataSize, sizeof(DumpTotal));
    //DumpSz
    fDst.Write(typeInfo^.RTTISz, sizeof(typeInfo^.RTTISz));
    Inc(DataSize, sizeof(typeInfo^.RTTISz));
    //FixupNum
    fDst.Write(FixupNum, sizeof(FixupNum));
    Inc(DataSize, sizeof(FixupNum));

    if (typeInfo^.RTTISz <> 0) then
    begin
      if (fIn <> Nil) then
      begin
        //Dump
        Inc(DataSize, WriteDump(fIn, fDst, typeInfo^.RTTIOfs, typeInfo^.RTTISz));
        fIn.Free;
        //Relocs
        Inc(DataSize, WriteRelocs(fDst, typeInfo^.Fixups, typeInfo^.RTTISz, typeInfo^.Name));
        //Fixups
        Inc(DataSize, WriteFixups(fDst, typeInfo^.Fixups));
      end;
    end;
    //FieldsTotal
    FieldsNum := typeInfo^.Fields.Count;
    FieldsTotal := 0;

    for m := 0 to FieldsNum -1 do
    begin
      linfo := PLOCALDECL_INFO(typeInfo^.Fields.Items[m]);
      Inc(FieldsTotal, sizeof(linfo^.Scope));
      Inc(FieldsTotal, sizeof(linfo^.Ndx));
      Inc(FieldsTotal, sizeof(linfo^.FCase));
      Inc(FieldsTotal, WriteString(Nil, linfo^.Name));
      Inc(FieldsTotal, WriteString(Nil, linfo^.TypeDef));
    end;
    Inc(FieldsTotal, sizeof(FieldsNum));   //FieldsNum

    fDst.Write(FieldsTotal, sizeof(FieldsTotal));
    Inc(DataSize, sizeof(FieldsTotal));
    //FieldsNum
    fDst.Write(FieldsNum, sizeof(FieldsNum));
    Inc(DataSize, sizeof(FieldsNum));
    //Fields
    for m := 0 to FieldsNum -1 do
    begin
      linfo := PLOCALDECL_INFO(typeInfo^.Fields.Items[m]);
      fDst.Write(linfo^.Scope, sizeof(linfo^.Scope));
      Inc(DataSize, sizeof(linfo^.Scope));
      fDst.Write(linfo^.Ndx, sizeof(linfo^.Ndx));
      Inc(DataSize, sizeof(linfo^.Ndx));
      fDst.Write(linfo^.FCase, sizeof(linfo^.FCase));
      Inc(DataSize, sizeof(linfo^.FCase));
      Inc(DataSize, WriteString(fDst, linfo^.Name));
      Inc(DataSize, WriteString(fDst, linfo^.TypeDef));
    end;
    //PropsTotal
    PropsNum := typeInfo^.Properties.Count;
    PropsTotal := 0;
    for m := 0 to PropsNum -1 do
    begin
      if (typeInfo^.Kind = drClassDef) Or (typeInfo^.Kind = drInterfaceDef) then
      begin
        pinfo := PPROPERTY_INFO(typeInfo^.Properties.Items[m]);
        Inc(PropsTotal, sizeof(pinfo^.Scope));
        Inc(PropsTotal, sizeof(pinfo^.Index));
        Inc(PropsTotal, sizeof(pinfo^.FDispId));
        Inc(PropsTotal, WriteString(Nil, pinfo^.Name));
        Inc(PropsTotal, WriteString(Nil, pinfo^.TypeDef));
        Inc(PropsTotal, WriteString(Nil, pinfo^.ReadName));
        Inc(PropsTotal, WriteString(Nil, pinfo^.WriteName));
        Inc(PropsTotal, WriteString(Nil, pinfo^.StoredName));
      end;
    end;
    Inc(PropsTotal, sizeof(PropsNum)); //PropsNum

    fDst.Write(PropsTotal, sizeof(PropsTotal));
    Inc(DataSize, sizeof(PropsTotal));
    //PropsNum
    fDst.Write(PropsNum, sizeof(PropsNum));
    Inc(DataSize, sizeof(PropsNum));
    //Props
    for m := 0 to PropsNum -1 do
    begin
      if (typeInfo^.Kind = drClassDef) Or (typeInfo^.Kind = drInterfaceDef) then
      begin
        pinfo := PPROPERTY_INFO(typeInfo^.Properties.Items[m]);
        fDst.Write(pinfo^.Scope, sizeof(pinfo^.Scope));
        Inc(DataSize, sizeof(pinfo^.Scope));
        fDst.Write(pinfo^.Index, sizeof(pinfo^.Index));
        Inc(DataSize, sizeof(pinfo^.Index));
        fDst.Write(pinfo^.FDispId, sizeof(pinfo^.FDispId));
        Inc(DataSize, sizeof(pinfo^.FDispId));
        Inc(DataSize, WriteString(fDst, pinfo^.Name));
        Inc(DataSize, WriteString(fDst, pinfo^.TypeDef));
        Inc(DataSize, WriteString(fDst, pinfo^.ReadName));
        Inc(DataSize, WriteString(fDst, pinfo^.WriteName));
        Inc(DataSize, WriteString(fDst, pinfo^.StoredName));
      end;
    end;
    //MethodsTotal
    MethodsNum := typeInfo^.Methods.Count;
    MethodsTotal := 0;
    for m := 0 to MethodsNum - 1 do
    begin
        minfo := PMETHODDECL_INFO(typeInfo^.Methods.Items[m]);
        Inc(MethodsTotal, sizeof(minfo^.Scope));
        Inc(MethodsTotal, sizeof(minfo^.MethodKind));
        Inc(MethodsTotal, WriteString(Nil, minfo^.Prototype));
    end;
    Inc(MethodsTotal, sizeof(MethodsNum)); //MethodsNum

    fDst.Write(MethodsTotal, sizeof(MethodsTotal));
    Inc(DataSize, sizeof(MethodsTotal));
    //MethodsNum
    fDst.Write(MethodsNum, sizeof(MethodsNum));
    Inc(DataSize, sizeof(MethodsNum));
    //Methods
    for m := 0 to MethodsNum -1 do
    begin
        minfo := PMETHODDECL_INFO(typeInfo^.Methods.Items[m]);
        fDst.Write(minfo^.Scope, sizeof(minfo^.Scope));
        Inc(DataSize, sizeof(minfo^.Scope));
        fDst.Write(minfo^.MethodKind, sizeof(minfo^.MethodKind));
        Inc(DataSize, sizeof(minfo^.MethodKind));
        Inc(DataSize, WriteString(fDst, minfo^.Prototype));
    end;
    typeInfo^.Size := DataSize;
    if (DataSize > MaxTypeDataSize) then
      MaxTypeDataSize := DataSize;
    Inc(CurrOffset, DataSize);
  end;
end;

procedure KBWriteVars(fDst:TFileStream);
var
  n:Integer;
  DataSize:Cardinal;
  vInfo:PVAR_INFO;
begin
  VarCount := VarList.Count;
  for n := 0 to VarCount -1 do
  begin
    vInfo := PVAR_INFO(VarList.Items[n]);
    vInfo^.ID := n;
    vInfo^.Offset := CurrOffset;

    DataSize := 0;
    //ModuleID
    fDst.Write(vInfo^.ModuleID, sizeof(vInfo^.ModuleID));
    Inc(DataSize, sizeof(vInfo^.ModuleID));
    //Name
    Inc(DataSize, WriteString(fDst, vInfo^.Name));
    //Type
    fDst.Write(vInfo^.FType, sizeof(vInfo^.FType));
    Inc(DataSize, sizeof(vInfo^.FType));
    //TypeDef
    Inc(DataSize, WriteString(fDst, vInfo^.TypeDef));
    //AbsName
    Inc(DataSize, WriteString(fDst, vInfo^.AbsName));

    vInfo^.Size := DataSize;
    if (DataSize > MaxVarDataSize) then
      MaxVarDataSize := DataSize;
    Inc(CurrOffset, DataSize);
  end;
end;

procedure KBWriteResStrings(fDst:TFileStream);
var
  ResStrLen:WORD;
  wptr:^WORD;
  cptr:PChar;
  n, m:Integer;
  DataSize:Cardinal;
  fIn:TFileStream;
  rsInfo:PRESSTR_INFO;
  ResStrBuf:array[0..4096] of BYTE;
begin
  ResStrCount := ResStrList.Count;
  for n := 0 to ResStrCount - 1 do
  begin
    rsInfo := PRESSTR_INFO(ResStrList.Items[n]);
    rsInfo^.ID := n;
    rsInfo^.Offset := CurrOffset;

    DataSize := 0;
    //ModuleID
    fDst.Write(rsInfo^.ModuleID, sizeof(rsInfo^.ModuleID));
    Inc(DataSize, sizeof(rsInfo^.ModuleID));
    //Name
    Inc(DataSize, WriteString(fDst, rsInfo^.Name));
    //TypeDef
    Inc(DataSize, WriteString(fDst, rsInfo^.TypeDef));
    //Context
    Inc(DataSize, WriteString(fDst, rsInfo^.Context));
    {*
    if (rsInfo^.AContext <> '') then
      Inc(DataSize, WriteString(fDst, rsInfo^.AContext))
    else if (rsInfo^.DumpSz <> 0) then
    begin
        fIn := Nil;
        for m := 0 to ModuleList.Count - 1 do
        begin
            ModuleInfo := PMODULE_INFO(ModuleList.Items[m]);
            if (ModuleInfo^.ModuleID = rsInfo^.ModuleID) then
            begin
                fIn := TFileStream.Create(ModuleInfo^.Filename, fmOpenRead);
                break;
            end;
        end;
        if (fIn <> Nil) then
        begin
            //Context
            fIn.Seek(rsInfo^.DumpOfs, soFromBeginning);
            fIn.Read(ResStrBuf, rsInfo^.DumpSz);
            fIn.Free;
            wptr := @ResStrBuf[4];
            ResStrLen := wptr^;
            cptr := @ResStrBuf[8];
            rsInfo^.AContext := Copy(cptr, 1, ResStrLen);
            Inc(DataSize, WriteString(fDst, rsInfo^.AContext));
        end;
    end;
    *}
    rsInfo^.Size := DataSize;
    if (DataSize > MaxResStrDataSize) then
      MaxResStrDataSize := DataSize;
    Inc(CurrOffset, DataSize);
  end;
end;

procedure KBWriteProcedures(fDst:TFileStream);
var
  n, m:Integer;
  ArgsNum, LocalsNum:WORD;
  DataSize, DumpTotal, FixupNum, ArgsTotal, LocalsTotal:Cardinal;
  fIn:TFileStream;
  pInfo:PPROCDECL_INFO;
  lInfo:PLOCALDECL_INFO;
begin
  fIn := Nil;
  ProcCount := ProcList.Count;
  for n := 0 to ProcCount - 1 do
  begin
    DataSize := 0;
    pInfo := PPROCDECL_INFO(ProcList.Items[n]);
    pInfo^.ID := n;
    pInfo^.Offset := CurrOffset;
    //ModuleID
    fDst.Write(pInfo^.ModuleID, sizeof(pInfo^.ModuleID));
    Inc(DataSize, sizeof(pInfo^.ModuleID));
    //Name
    Inc(DataSize, WriteString(fDst, pInfo^.Name));
    //Embedded
    fDst.Write(pInfo^.Embedded, sizeof(pInfo^.Embedded));
    Inc(DataSize, sizeof(pInfo^.Embedded));
    //DumpType
    fDst.Write(pInfo^.DumpType, sizeof(pInfo^.DumpType));
    Inc(DataSize, sizeof(pInfo^.DumpType));
    //MethodKind
    fDst.Write(pInfo^.MethodKind, sizeof(pInfo^.MethodKind));
    Inc(DataSize, sizeof(pInfo^.MethodKind));
    //CallKind
    fDst.Write(pInfo^.CallKind, sizeof(pInfo^.CallKind));
    Inc(DataSize, sizeof(pInfo^.CallKind));
    //VProc
    fDst.Write(pInfo^.VProc, sizeof(pInfo^.VProc));
    Inc(DataSize, sizeof(pInfo^.VProc));
    //TypeDef
    Inc(DataSize, WriteString(fDst, pInfo^.TypeDef));
    //DumpTotal
    DumpTotal := 0;
    if (pInfo^.DumpSz <> 0) then
    begin
      fIn := Nil;
      for m := 0 to ModuleList.Count - 1 do
      begin
        ModuleInfo := PMODULE_INFO(ModuleList.Items[m]);
        if (ModuleInfo^.ModuleID = pInfo^.ModuleID) then
        begin
          fIn := TFileStream.Create(ModuleInfo^.Filename, fmOpenRead);
          break;
        end;
      end;
      if (fIn <> Nil) then
      begin
        //Dump
        Inc(DumpTotal, pInfo^.DumpSz);
        //Relocs
        Inc(DumpTotal, pInfo^.DumpSz);
        //Fixups
        Inc(DumpTotal, WriteFixups(Nil, pInfo^.Fixups));
      end;
    end;
    Inc(DumpTotal, sizeof(pInfo^.DumpSz)); //DumpSz
    FixupNum := pInfo^.Fixups.Count;
    Inc(DumpTotal, sizeof(FixupNum));      //FixupNum

    fDst.Write(DumpTotal, sizeof(DumpTotal));
    Inc(DataSize, sizeof(DumpTotal));
    //DumpSz
    fDst.Write(pInfo^.DumpSz, sizeof(pInfo^.DumpSz));
    Inc(DataSize, sizeof(pInfo^.DumpSz));
    //FixupNum
    fDst.Write(FixupNum, sizeof(FixupNum));
    Inc(DataSize, sizeof(FixupNum));

    if (pInfo^.DumpSz <> 0) then
    begin
      if (fIn <> Nil) then
      begin
        //Dump
        Inc(DataSize, WriteDump(fIn, fDst, pInfo^.DumpOfs, pInfo^.DumpSz));
        fIn.Free;
        //Relocs
        Inc(DataSize, WriteRelocs(fDst, pInfo^.Fixups, pInfo^.DumpSz, pInfo^.Name));
        //Fixups
        Inc(DataSize, WriteFixups(fDst, pInfo^.Fixups));
      end;
    end;

    //ArgsTotal
    ArgsNum := pInfo^.Args.Count;
    ArgsTotal := 0;
    for m := 0 to ArgsNum - 1 do
    begin
      lInfo := PLOCALDECL_INFO(pInfo^.Args.Items[m]);
      Inc(ArgsTotal, sizeof(lInfo^.Tag));
      Inc(ArgsTotal, sizeof(lInfo^.LocFlags));
      Inc(ArgsTotal, sizeof(lInfo^.Ndx));
      Inc(ArgsTotal, WriteString(Nil, lInfo^.Name));
      Inc(ArgsTotal, WriteString(Nil, lInfo^.TypeDef));
    end;
    Inc(ArgsTotal, sizeof(ArgsNum));

    fDst.Write(ArgsTotal, sizeof(ArgsTotal));
    Inc(DataSize, sizeof(ArgsTotal));
    //ArgsNum
    fDst.Write(ArgsNum, sizeof(ArgsNum));
    Inc(DataSize, sizeof(ArgsNum));
    //Args
    for m := 0 to ArgsNum - 1 do
    begin
      lInfo := PLOCALDECL_INFO(pInfo^.Args.Items[m]);

      fDst.Write(lInfo^.Tag, sizeof(lInfo^.Tag));
      Inc(DataSize, sizeof(lInfo^.Tag));
      fDst.Write(lInfo^.LocFlags, sizeof(lInfo^.LocFlags));
      Inc(DataSize, sizeof(lInfo^.LocFlags));
      fDst.Write(lInfo^.Ndx, sizeof(lInfo^.Ndx));
      Inc(DataSize, sizeof(lInfo^.Ndx));
      Inc(DataSize, WriteString(fDst, lInfo^.Name));
      Inc(DataSize, WriteString(fDst, lInfo^.TypeDef));
    end;
    //LocalsTotal
    {*
    LocalsNum := pInfo^.Locals.Count;
    LocalsTotal := 0;
    for m := 0 to LocalsNum - 1 do
    begin
      lInfo := PLOCALDECL_INFO(pInfo^.Locals.Items[m]);
      Inc(LocalsTotal, sizeof(lInfo^.Tag));
      Inc(LocalsTotal, sizeof(lInfo^.LocFlags));
      Inc(LocalsTotal, sizeof(lInfo^.Ndx));
      Inc(LocalsTotal, WriteString(Nil, lInfo^.Name));
      Inc(LocalsTotal, WriteString(Nil, lInfo^.TypeDef));
      Inc(LocalsTotal, WriteString(Nil, lInfo^.AbsName));
    end;
    Inc(LocalsTotal, sizeof(LocalsNum));

    fDst.Write(LocalsTotal, sizeof(LocalsTotal));
    Inc(DataSize, sizeof(LocalsTotal));
    //LocalsNum
    fDst.Write(LocalsNum, sizeof(LocalsNum));
    Inc(DataSize, sizeof(LocalsNum));
    //Locals
    for m := 0 to LocalsNum - 1 do
    begin
      lInfo := PLOCALDECL_INFO(pInfo^.Locals.Items[m]);
      fDst.Write(lInfo^.Tag, sizeof(lInfo^.Tag));
      Inc(DataSize, sizeof(lInfo^.Tag));
      fDst.Write(lInfo^.LocFlags, sizeof(lInfo^.LocFlags));
      Inc(DataSize, sizeof(lInfo^.LocFlags));
      fDst.Write(lInfo^.Ndx, sizeof(lInfo^.Ndx));
      Inc(DataSize, sizeof(lInfo^.Ndx));
      Inc(DataSize, WriteString(fDst, lInfo^.Name));
      Inc(DataSize, WriteString(fDst, lInfo^.TypeDef));
      Inc(DataSize, WriteString(fDst, lInfo^.AbsName));
    end;
    *}
    pInfo^.Size := DataSize;
    if (DataSize > MaxProcDataSize) then MaxProcDataSize := DataSize;
    Inc(CurrOffset, DataSize);
  end;
end;

procedure KBWriteOffsets(fDst:TFileStream);
var
  n, cn:Integer;
  ConstInfo:PCONST_INFO;
  TypeInfo:PTYPE_INFO;
  VarInfo:PVAR_INFO;
  ResInfo:PRESSTR_INFO;
  ProcInfo:PPROCDECL_INFO;
  Offsets:array of OFFSETSINFO;
begin
  //Modules
  fDst.Write(ModuleCount, sizeof(ModuleCount));
  fDst.Write(MaxModuleDataSize, sizeof(MaxModuleDataSize));
  SetLength(Offsets, ModuleCount);
  for n := 0 to ModuleCount - 1 do
  begin
    ModuleInfo := PMODULE_INFO(ModuleList.Items[n]);
    Offsets[n].Offset := ModuleInfo^.Offset;
    Offsets[n].Size := ModuleInfo^.Size;
  end;
  ModuleList.Sort(CompareModulesByID);
  for n := 0 to ModuleCount - 1 do
  begin
    ModuleInfo := PMODULE_INFO(ModuleList.Items[n]);
    Offsets[n].ModId := ModuleInfo^.ID;
  end;
  ModuleList.Sort(CompareModulesByName);
  for n := 0 to ModuleCount - 1 do
  begin
    ModuleInfo := PMODULE_INFO(ModuleList.Items[n]);
    Offsets[n].NamId := ModuleInfo^.ID;
  end;
  for n := 0 to ModuleCount - 1 do
  begin
    fDst.Write(Offsets[n].Offset, sizeof(Offsets[n].Offset));
    fDst.Write(Offsets[n].Size, sizeof(Offsets[n].Size));
    fDst.Write(Offsets[n].ModId, sizeof(Offsets[n].ModId));
    fDst.Write(Offsets[n].NamId, sizeof(Offsets[n].NamId));
  end;
  Offsets := nil;
  //Consts
  fDst.Write(ConstCount, sizeof(ConstCount));
  fDst.Write(MaxConstDataSize, sizeof(MaxConstDataSize));
  SetLength(Offsets, ConstCount);
  cn := 0;
  for n := 0 to ConstList.Count - 1 do
  begin
    ConstInfo := PCONST_INFO(ConstList.Items[n]);
    if (constInfo.Skip) then continue;

    Offsets[cn].Offset := ConstInfo^.Offset;
    Offsets[cn].Size := ConstInfo^.Size;
    Inc(cn);
  end;
  ConstList.Sort(CompareConstsByID);
  cn := 0;
  for n := 0 to ConstList.Count - 1 do
  begin
    ConstInfo := PCONST_INFO(ConstList.Items[n]);
    if (constInfo.Skip) then continue;

    Offsets[cn].ModId := ConstInfo^.ID;
    Inc(cn);
  end;
  ConstList.Sort(CompareConstsByName);
  cn := 0;
  for n := 0 to ConstList.Count - 1 do
  begin
      ConstInfo := PCONST_INFO(ConstList.Items[n]);
      if (constInfo.Skip) then continue;

      Offsets[cn].NamId := ConstInfo^.ID;
      Inc(cn);
  end;
  for n := 0 to ConstCount - 1 do
  begin
    fDst.Write(Offsets[n].Offset, sizeof(Offsets[n].Offset));
    fDst.Write(Offsets[n].Size, sizeof(Offsets[n].Size));
    fDst.Write(Offsets[n].ModId, sizeof(Offsets[n].ModId));
    fDst.Write(Offsets[n].NamId, sizeof(Offsets[n].NamId));
  end;
  Offsets := nil;
  //Types
  fDst.Write(TypeCount, sizeof(TypeCount));
  fDst.Write(MaxTypeDataSize, sizeof(MaxTypeDataSize));
  SetLength(Offsets, TypeCount);
  for n := 0 to TypeCount - 1 do
  begin
      TypeInfo := PTYPE_INFO(TypeList.Items[n]);
      Offsets[n].Offset := TypeInfo^.Offset;
      Offsets[n].Size := TypeInfo^.Size;
  end;
  TypeList.Sort(CompareTypesByID);
  for n := 0 to TypeCount - 1 do
  begin
      TypeInfo := PTYPE_INFO(TypeList.Items[n]);
      Offsets[n].ModId := TypeInfo^.ID;
  end;
  TypeList.Sort(CompareTypesByName);
  for n := 0 to TypeCount - 1 do
  begin
      TypeInfo := PTYPE_INFO(TypeList.Items[n]);
      Offsets[n].NamId := TypeInfo^.ID;
  end;
  for n := 0 to TypeCount - 1 do
  begin
    fDst.Write(Offsets[n].Offset, sizeof(Offsets[n].Offset));
    fDst.Write(Offsets[n].Size, sizeof(Offsets[n].Size));
    fDst.Write(Offsets[n].ModId, sizeof(Offsets[n].ModId));
    fDst.Write(Offsets[n].NamId, sizeof(Offsets[n].NamId));
  end;
  Offsets := nil;
  //Vars
  fDst.Write(VarCount, sizeof(VarCount));
  fDst.Write(MaxVarDataSize, sizeof(MaxVarDataSize));
  SetLength(Offsets, VarCount);
  for n := 0 to VarCount - 1 do
  begin
      VarInfo := PVAR_INFO(VarList.Items[n]);
      Offsets[n].Offset := VarInfo^.Offset;
      Offsets[n].Size := VarInfo^.Size;
  end;
  VarList.Sort(CompareVarsByID);
  for n := 0 to VarCount - 1 do
  begin
      VarInfo := PVAR_INFO(VarList.Items[n]);
      Offsets[n].ModId := VarInfo^.ID;
  end;
  VarList.Sort(CompareVarsByName);
  for n := 0 to VarCount - 1 do
  begin
      VarInfo := PVAR_INFO(VarList.Items[n]);
      Offsets[n].NamId := VarInfo^.ID;
  end;
  for n := 0 to VarCount - 1 do
  begin
    fDst.Write(Offsets[n].Offset, sizeof(Offsets[n].Offset));
    fDst.Write(Offsets[n].Size, sizeof(Offsets[n].Size));
    fDst.Write(Offsets[n].ModId, sizeof(Offsets[n].ModId));
    fDst.Write(Offsets[n].NamId, sizeof(Offsets[n].NamId));
  end;
  Offsets := nil;
  //ResStrings
  fDst.Write(ResStrCount, sizeof(ResStrCount));
  fDst.Write(MaxResStrDataSize, sizeof(MaxResStrDataSize));
  SetLength(Offsets, ResStrCount);
  for n := 0 to ResStrCount - 1 do
  begin
      ResInfo := PRESSTR_INFO(ResStrList.Items[n]);
      Offsets[n].Offset := ResInfo^.Offset;
      Offsets[n].Size := ResInfo^.Size;
  end;
  ResStrList.Sort(CompareResStrsByID);
  for n := 0 to ResStrCount - 1 do
  begin
      ResInfo := PRESSTR_INFO(ResStrList.Items[n]);
      Offsets[n].ModId := ResInfo^.ID;
  end;
  ResStrList.Sort(CompareResStrsByName);
  for n := 0 to ResStrCount - 1 do
  begin
      ResInfo := PRESSTR_INFO(ResStrList.Items[n]);
      Offsets[n].NamId := ResInfo^.ID;
  end;
  for n := 0 to ResStrCount - 1 do
  begin
    fDst.Write(Offsets[n].Offset, sizeof(Offsets[n].Offset));
    fDst.Write(Offsets[n].Size, sizeof(Offsets[n].Size));
    fDst.Write(Offsets[n].ModId, sizeof(Offsets[n].ModId));
    fDst.Write(Offsets[n].NamId, sizeof(Offsets[n].NamId));
  end;
  Offsets := nil;
  //Procs
  fDst.Write(ProcCount, sizeof(ProcCount));
  fDst.Write(MaxProcDataSize, sizeof(MaxProcDataSize));
  SetLength(Offsets, ProcCount);
  for n := 0 to ProcCount - 1 do
  begin
      ProcInfo := PPROCDECL_INFO(ProcList.Items[n]);
      Offsets[n].Offset := ProcInfo^.Offset;
      Offsets[n].Size := ProcInfo^.Size;
  end;
  ProcList.Sort(CompareProcsByID);
  for n := 0 to ProcCount - 1 do
  begin
      ProcInfo := PPROCDECL_INFO(ProcList.Items[n]);
      Offsets[n].ModId := ProcInfo^.ID;
  end;
  ProcList.Sort(CompareProcsByName);
  for n := 0 to ProcCount - 1 do
  begin
      ProcInfo := PPROCDECL_INFO(ProcList.Items[n]);
      Offsets[n].NamId := ProcInfo^.ID;
  end;
  for n := 0 to ProcCount - 1 do
  begin
    fDst.Write(Offsets[n].Offset, sizeof(Offsets[n].Offset));
    fDst.Write(Offsets[n].Size, sizeof(Offsets[n].Size));
    fDst.Write(Offsets[n].ModId, sizeof(Offsets[n].ModId));
    fDst.Write(Offsets[n].NamId, sizeof(Offsets[n].NamId));
  end;
  Offsets := nil;
  //
  fDst.Write(CurrOffset, sizeof(CurrOffset));
end;

procedure ProcessFiles;
var
  n, res:Integer;
  sr:TSearchRec;
  list:TStringList;
begin
  if (KBUnitsList = '*') then
  begin
    res := FindFirst('*.dcu', faArchive, sr);
    while (res = 0) do
    begin
      ProcessFile(sr.Name);
      res := FindNext(sr);
    end;
  end
  else
  begin
    list := TStringList.Create;
    list.LoadFromFile(KBUnitsList);
    for n := 0 to list.Count - 1 do
    begin
      ProcessFile(list.Strings[n]);
    end;
    list.Free;
  end;
end;
//_crypto

begin
  DecimalSeparator := '.';
  if not ProcessParms then begin
    Writeln('Call this program with -? or -h parameters for help on usage.');//WriteUsage;
    Exit;
  end ;
  //crypto_
  KBStream := TFileStream.Create('kb.bin', fmCreate);
  CurrOffset := 0;
  KBStream.Write(Pointer(KBSignature)^, Length(KBSignature) + 1);
  Inc(CurrOffset, Length(KBSignature) + 1);
  KBStream.Write(KBIsMSIL, sizeof(KBIsMSIL));
  Inc(CurrOffset, sizeof(KBIsMSIL));
  KBStream.Write(KBFVer, sizeof(KBFVer));
  Inc(CurrOffset, sizeof(KBFVer));
  KBStream.Write(KBCRC, sizeof(KBCRC));
  Inc(CurrOffset, sizeof(KBCRC));
  KBStream.Write(KBDescription, 256);
  Inc(CurrOffset, 256);
  KBStream.Write(KBVersion, sizeof(KBVersion));
  Inc(CurrOffset, sizeof(KBVersion));
  KBStream.Write(KBCreateDT, sizeof(KBCreateDT));
  Inc(CurrOffset, sizeof(KBCreateDT));
  KBStream.Write(KBLastModifyDT, sizeof(KBLastModifyDT));
  Inc(CurrOffset, sizeof(KBLastModifyDT));

  ModuleList := TList.Create;
  ConstList := TList.Create;
  TypeList := TList.Create;
  VarList := TList.Create;
  ResStrList := TList.Create;
  ProcList := TList.Create;

  FNRes := '';
  ProcessFiles;

  KBWriteModules(KBStream);
  KBWriteConstants(KBStream);
  KBWriteTypes(KBStream);
  KBWriteVars(KBStream);
  KBWriteResStrings(KBStream);
  KBWriteProcedures(KBStream);

  KBWriteOffsets(KBStream);
  KBStream.Free;

  ModuleList.Free;
  ConstList.Free;
  TypeList.Free;
  VarList.Free;
  ResStrList.Free;
  ProcList.Free;

  Halt;
  //_crypto
  Halt(ProcessFile(DCUName));
end.
