/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "symtable.h"

#include "irgen.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/raw_ostream.h"

Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    printf("\n");
}

llvm::Value* BreakStmt::Emit() {
    llvm::BranchInst::Create(irgen->breakStack->top(), irgen->GetBasicBlock());
    //irgen->SetBasicBlock(llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction()));
    return NULL;
}

llvm::Value* ContinueStmt::Emit() {
    llvm::BranchInst::Create(irgen->continueStack->top(), irgen->GetBasicBlock());
    irgen->SetBasicBlock(llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction()));
    return NULL;
}

llvm::Value* WhileStmt::Emit() {
    // cuz we need a new scope yoooo :)
    symtab->push();

    // so we don't have to keep typing irgen->GetBasicBlock()
    llvm::BasicBlock *block = irgen->GetBasicBlock();

    // create and push basic block for footer, step, body and header
    llvm::BasicBlock *footerBlock = llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction());
    llvm::BasicBlock *bodyBlock = llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction());
    llvm::BasicBlock *headerBlock = llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction());

    irgen->footStack->push(footerBlock);
    irgen->breakStack->push(footerBlock);
    irgen->continueStack->push(bodyBlock);

    // create a branch to terminate current basic block and start loop header
    llvm::BranchInst::Create(headerBlock, block);
    headerBlock->moveAfter(block);

    // IR gen for headBB
    irgen->SetBasicBlock(headerBlock);

    // emit for test
    llvm::BranchInst::Create(bodyBlock, footerBlock, test->Emit(), headerBlock);

    // emit for body
    if (body) {
        irgen->SetBasicBlock(bodyBlock);
        body->Emit();
    }

    // check if there is a terminator instruction else create one
    if (!bodyBlock->getTerminator()) {
        llvm::BranchInst::Create(headerBlock, bodyBlock);
    }
    footerBlock->moveAfter(bodyBlock);

    irgen->footStack->pop();
    irgen->breakStack->pop();
    irgen->continueStack->pop();
    symtab->pop();
    return NULL;
}

llvm::Value* ForStmt::Emit() {
    // cuz we need a new scope yoooo :)
    symtab->push();

    // so we don't have to keep typing irgen->GetBasicBlock()
    llvm::BasicBlock *block = irgen->GetBasicBlock();

    // create and push basic block for footer, step, body and header
    llvm::BasicBlock *footerBlock = llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction());
    llvm::BasicBlock *stepBlock = llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction());
    llvm::BasicBlock *bodyBlock = llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction());
    llvm::BasicBlock *headerBlock = llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction());

    irgen->footStack->push(footerBlock);
    irgen->breakStack->push(footerBlock);
    irgen->continueStack->push(stepBlock);

    // emit for initialization if any
    if (init) {
        init->Emit();
    }

    // create a branch to terminate current basic block and start loop header
    llvm::BranchInst::Create(headerBlock, block);
    headerBlock->moveAfter(block);

    // IR gen for headBB
    irgen->SetBasicBlock(bodyBlock);

    // emit for test
    llvm::BranchInst::Create(bodyBlock, footerBlock, test->Emit(), headerBlock);

    // jump to footer

    // emit for body
    if (body) {
        irgen->SetBasicBlock(bodyBlock);
        body->Emit();
    }

    // check if there is a terminator instruction else create one
    if (!bodyBlock->getTerminator()) {
        llvm::BranchInst::Create(stepBlock, bodyBlock);
    }

    // emit for step
    irgen->SetBasicBlock(stepBlock);
    stepBlock->moveAfter(bodyBlock);
    if (step) {
        step->Emit();
    }
    llvm::BranchInst::Create(headerBlock, stepBlock);
    irgen->SetBasicBlock(footerBlock);
    footerBlock->moveAfter(stepBlock);

    // create terminator for step*/
    if (!stepBlock->getTerminator()) {
        llvm::BranchInst::Create(footerBlock, stepBlock);
    }

    /*if(footerBlock->getSinglePredecessor() == footerBlock->getSingleSuccessor()) {
      new llvm::UnreachableInst(*irgen->GetContext(), footerBlock);
    }
    else {*/
      irgen->SetBasicBlock(footerBlock);
      llvm::BasicBlock* pfootBlock = irgen->footStack->top();
      if(irgen->footStack->size() != 0){
        if(pfootBlock != footerBlock){
          irgen->footStack->pop();
          if(!pfootBlock->getTerminator()){
            llvm::BranchInst::Create(stepBlock, pfootBlock);
          }
        }
      }
    //}

    irgen->footStack->pop();
    irgen->breakStack->pop();
    irgen->continueStack->pop();
    symtab->pop();
    return NULL;
}

llvm::Value* Program::Emit() {
    llvm::Module *mod = irgen->GetOrCreateModule("mod.bc");

    if (decls->NumElements() > 0) {
        for (int i = 0; i < decls->NumElements(); i++) {
            Decl *declarations = decls->Nth(i);
            if (declarations) {
                declarations->Emit();
            }
        }
    }

    llvm::WriteBitcodeToFile(mod, llvm::outs());
    return NULL;
}

llvm::Value* IfStmt::Emit() {
    llvm::BasicBlock* block = irgen->GetBasicBlock();

    // emit for test condition
    llvm::Value* check = test->Emit();
    symtab->push();

    // create and push basic block for footer
    llvm::BasicBlock *footerBlock =
        llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction());
    irgen->footStack->push(footerBlock);

    // if "if statement" has a corresponding else body, create basic block for elsebody and push
    llvm::BasicBlock *elseBlock;
    if (elseBody) {
        elseBlock = llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction());
        //irgen->elseStack->push(elseBlock);
    }

    // create basic block for then part
    llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(*irgen->GetContext(), "", irgen->GetFunction());

    // create branch instructions with all parameters calculated above
    llvm::BranchInst::Create(thenBlock, elseBody ? elseBlock : footerBlock, check, block);
    thenBlock->moveAfter(block);
    // emit code for point then-basicblock
    irgen->SetBasicBlock(thenBlock);
    body->Emit();
    if (elseBlock) elseBlock->moveAfter(thenBlock);

    // create jump to foot-basicblock
    if (!thenBlock->getTerminator()) {
        llvm::BranchInst::Create(footerBlock, thenBlock);
    }

    symtab->pop();
    symtab->push();

    // emit code for point else-basicblock
    if (elseBody) {
        irgen->SetBasicBlock(elseBlock);
        elseBody->Emit();
        // create jump for else-basicblock
        if (!elseBlock->getTerminator()) {
            llvm::BranchInst::Create(footerBlock, elseBlock);
        }
    }
    if (elseBlock) footerBlock->moveAfter(elseBlock);
    else footerBlock->moveAfter(thenBlock);
    irgen->footStack->pop();

    symtab->pop();
    return NULL;
}

llvm::Value* StmtBlock::Emit() {
    if (decls->NumElements() > 0) {
	for (int i = 0; i < decls->NumElements(); i++) {
        Decl *d = decls->Nth(i);
			d->Emit();
		}
	}

    if (stmts->NumElements() > 0) {
	   for (int i = 0; i < stmts->NumElements(); i++) {
	       Stmt *s = stmts->Nth(i);
	       StmtBlock *stmtBlock = dynamic_cast<StmtBlock*>(s);
		   if(stmtBlock) symtab->push();
		   s->Emit();
		   if(stmtBlock) symtab->pop();
	    }
	}

    return NULL;
}

llvm::Value* DeclStmt::Emit() {
    if (decl) {
        decl->Emit();
    }

    return NULL;
}

llvm::Value* ReturnStmt::Emit() {
    llvm::Function *function = irgen->GetFunction();
    llvm::BasicBlock *block = irgen->GetBasicBlock();
    llvm::ReturnInst *toRet;

    // return type is void
    if (function->getReturnType() == llvm::Type::getVoidTy(*irgen->GetContext())) {
        toRet = llvm::ReturnInst::Create(*irgen->GetContext(), block);
    }
    else {
        toRet = llvm::ReturnInst::Create(*irgen->GetContext(), expr->Emit(), block);
    }
    return toRet;
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    stmts->PrintAll(indentLevel+1);
}

DeclStmt::DeclStmt(Decl *d) {
    Assert(d != NULL);
    (decl=d)->SetParent(this);
}

void DeclStmt::PrintChildren(int indentLevel) {
    decl->Print(indentLevel+1);
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) {
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this);
    (body=b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) {
    Assert(i != NULL && t != NULL && b != NULL);
    (init=i)->SetParent(this);
    step = s;
    if ( s )
      (step=s)->SetParent(this);
}

void ForStmt::PrintChildren(int indentLevel) {
    init->Print(indentLevel+1, "(init) ");
    test->Print(indentLevel+1, "(test) ");
    if ( step )
      step->Print(indentLevel+1, "(step) ");
    body->Print(indentLevel+1, "(body) ");
}

void WhileStmt::PrintChildren(int indentLevel) {
    test->Print(indentLevel+1, "(test) ");
    body->Print(indentLevel+1, "(body) ");
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) {
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::PrintChildren(int indentLevel) {
    if (test) test->Print(indentLevel+1, "(test) ");
    if (body) body->Print(indentLevel+1, "(then) ");
    if (elseBody) elseBody->Print(indentLevel+1, "(else) ");
}


ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) {
    expr = e;
    if (e != NULL) expr->SetParent(this);
}

void ReturnStmt::PrintChildren(int indentLevel) {
    if ( expr )
      expr->Print(indentLevel+1);
}

SwitchLabel::SwitchLabel(Expr *l, Stmt *s) {
    Assert(l != NULL && s != NULL);
    (label=l)->SetParent(this);
    (stmt=s)->SetParent(this);
}

SwitchLabel::SwitchLabel(Stmt *s) {
    Assert(s != NULL);
    label = NULL;
    (stmt=s)->SetParent(this);
}

void SwitchLabel::PrintChildren(int indentLevel) {
    if (label) label->Print(indentLevel+1);
    if (stmt)  stmt->Print(indentLevel+1);
}

SwitchStmt::SwitchStmt(Expr *e, List<Stmt *> *c, Default *d) {
    Assert(e != NULL && c != NULL && c->NumElements() != 0 );
    (expr=e)->SetParent(this);
    (cases=c)->SetParentAll(this);
    def = d;
    if (def) def->SetParent(this);
}

void SwitchStmt::PrintChildren(int indentLevel) {
    if (expr) expr->Print(indentLevel+1);
    if (cases) cases->PrintAll(indentLevel+1);
    if (def) def->Print(indentLevel+1);
}
