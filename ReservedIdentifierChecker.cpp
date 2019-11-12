#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/BugReporter/PathDiagnostic.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"

#include <string>
#include <regex>

using namespace clang;
using namespace ento;

namespace {
class DeclVisitor : public clang::RecursiveASTVisitor<DeclVisitor> {
  AnalysisManager& MGR;
  BugReporter &BR;
  AnalysisDeclContext* AC;
  const CheckerBase* CB;
  
public :
  explicit DeclVisitor (BugReporter &br, AnalysisManager& mgr,  AnalysisDeclContext *ac, const CheckerBase *cb)
	: MGR(mgr), BR(br), AC(ac), CB(cb) {}
  
  // Declaration visitor methods
  bool VisitVarDecl(const VarDecl *VD);
  bool VisitFunctionDecl(const FunctionDecl *FD);

private:
  bool isReserved(std::string DeclName);
  std::string getHeaderName(std::string headerPath);
  void reportBug(const Decl *D);
  void reportBug(std::string Msg, const Decl *D);
  void reportBug(StringRef Msg, const Decl *D);
};
}

// Declaration visitor methods
bool DeclVisitor::VisitFunctionDecl(const FunctionDecl *FD) {
  SourceManager &sm= MGR.getSourceManager();
  if (!sm.isInMainFile(FD->getLocation()))
      return true;
  FileID mainFileID = sm.getMainFileID();
  for (auto it = sm.fileinfo_begin(); it != sm.fileinfo_end(); it++) {
      SourceLocation includeLoc = sm.getIncludeLoc(sm.translateFile(it->first));
      if (includeLoc.isValid() && sm.isInFileID(includeLoc, mainFileID))
          reportBug(it->first->getName(), FD);
  }

  return true;
}

bool DeclVisitor::VisitVarDecl(const VarDecl *VD) {
  if (VD->getLinkageInternal() == ExternalLinkage)
      reportBug(VD);
    return true;
}    

std::string DeclVisitor::getHeaderName(std::string headerPath) {
    std::regex reg("[a-z]+\\.h");
    std::smatch match_reg;
    std::regex_search(headerPath, match_reg, reg);

    return match_reg.str();
}

bool DeclVisitor::isReserved(std::string DeclName) {
    if (DeclName.starts_with("_")) // C++20 
        return true;
}

void DeclVisitor::reportBug(const Decl *D) {
  PathDiagnosticLocation ELoc =
      PathDiagnosticLocation::createBegin(D, BR.getSourceManager());
  BR.EmitBasicReport(AC->getDecl(), CB,
		  "", 
		  "RESERVED_IDENTIFIER",
		  "WRONG IDENTIFIER USAGE", 
		  ELoc, D->getSourceRange());
}

void DeclVisitor::reportBug(std::string Msg, const Decl *D) {
  StringRef MSG = StringRef(Msg);
  PathDiagnosticLocation ELoc =
      PathDiagnosticLocation::createBegin(D, BR.getSourceManager());
  BR.EmitBasicReport(AC->getDecl(), CB,
		  "", 
		  "RESERVED_IDENTIFIER",
		  //"WRONG IDENTIFIER USAGE", 
          MSG,
		  ELoc, D->getSourceRange());
}

void DeclVisitor::reportBug(StringRef MSG, const Decl *D) {
  PathDiagnosticLocation ELoc =
      PathDiagnosticLocation::createBegin(D, BR.getSourceManager());
  BR.EmitBasicReport(AC->getDecl(), CB,
		  "", 
		  "RESERVED_IDENTIFIER",
		  //"WRONG IDENTIFIER USAGE", 
          MSG,
		  ELoc, D->getSourceRange());
}

namespace {
class ReservedIdentifierChecker: public Checker<check::ASTCodeBody> {
public :
  void checkASTCodeBody(const Decl *D, AnalysisManager& mgr,
                        BugReporter &BR) const {
    DeclVisitor Visitor(BR, mgr, mgr.getAnalysisDeclContext(D), this);
    Visitor.TraverseDecl(const_cast<Decl *>(D));
  }
};
}

void ento::registerReservedIdentifierChecker(CheckerManager &mgr) {
  mgr.registerChecker<ReservedIdentifierChecker>();
}

bool ento::shouldRegisterReservedIdentifierChecker(const LangOptions &LO) {
   return true;
}
