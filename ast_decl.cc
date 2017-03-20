/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "symtable.h"

Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this);
}

llvm::Value* VarDecl::Emit() {
    Symbol *sym = symtab->findInCurrentScope(GetIdentifier()->GetName());
    llvm::Module *mod = irgen->GetOrCreateModule("mod.bc");

    if (!sym) {

        sym = new Symbol(GetIdentifier()->GetName(), this, E_VarDecl, 0);
        //VarDecl *vDecl = dynamic_cast<VarDecl*>(sym->decl);
        //cerr << endl << "name: " << sym->name;

        if (symtab->isGlobalScope()) {
            sym->value = new llvm::GlobalVariable(*mod,
                getLLVMType(),
                false, llvm::GlobalValue::ExternalLinkage,
                llvm::Constant::getNullValue(getLLVMType()),
                sym->name);
        }
        else {
            sym->value = new llvm::AllocaInst(getLLVMType(), sym->name, irgen->GetBasicBlock());
        }
        symtab->insert(*sym);
    }

    return sym->value;
}

llvm::Value* FnDecl::Emit() {
    // get or insert function
    // specify function type
    // set names for arguments / formals
    // create basic blocks

    // search the symbol table for function
    Symbol *sym = symtab->findInCurrentScope(GetIdentifier()->GetName());
    llvm::Module *mod = irgen->GetOrCreateModule("mod.bc");

    // if function does not exist, insert it into the symbol table
    if (!sym) {
        sym = new Symbol(GetIdentifier()->GetName(), this, E_FunctionDecl, 0);
        //symtab->insert(*sym);
    }

    // start function scope
    symtab->push();

    // typeList is a list of argument types
    std::vector<llvm::Type*> typeList;

    // Add args
	if (GetFormals()->NumElements() > 0) {
	    for (int i = 0; i < GetFormals()->NumElements(); i++) {
		    VarDecl *d = GetFormals()->Nth(i);
            typeList.push_back(d->getLLVMType());
		}
	}

    // convert the list of argumet types
    llvm::ArrayRef<llvm::Type*> *params = new llvm::ArrayRef<llvm::Type*>(typeList);
    llvm::FunctionType *functionType = llvm::FunctionType::get(getLLVMType(), *params, false);

    // Make function
    llvm::Function *fun = llvm::cast<llvm::Function>
        (mod->getOrInsertFunction(sym->name, functionType));

    irgen->SetFunction(fun);

    // Create BasicBlock(s)
    llvm::BasicBlock *block = llvm::BasicBlock::Create(*irgen->GetContext(), "entry", fun);
    irgen->SetBasicBlock(block);

    // Set names of args
    llvm::Function::arg_iterator argit;
    int index = 0;
    for (argit = fun->arg_begin(); argit != fun->arg_end(); argit++) {
        VarDecl *d = GetFormals()->Nth(index);
        argit->setName(d->GetIdentifier()->GetName());
        new llvm::StoreInst(argit, d->Emit(), irgen->GetBasicBlock());
        index++;
    }

    body->Emit();

    if (!block->getTerminator()) {
        llvm::ReturnInst::Create(*irgen->GetContext(), block);
    }

    symtab->pop();
    sym->value = fun;
		symtab->insert(*sym);
    return fun;
}

llvm::Type * VarDecl::getLLVMType() const {
		Type * type = GetType();
		ArrayType *aType = dynamic_cast<ArrayType*>(type);
		if (aType) {
				type = aType->GetElemType();
		}

    if (type == Type::intType) return irgen->GetIntType();
    else if (type == Type::boolType) return irgen->GetBoolType();
    else if (type == Type::floatType) return irgen->GetFloatType();
    else if (type == Type::voidType) return llvm::Type::getVoidTy(*irgen->GetContext());
    else if (type == Type::vec2Type) return llvm::VectorType::get(llvm::Type::getFloatTy(*irgen->GetContext()), 2);
    else if (type == Type::vec3Type) return llvm::VectorType::get(llvm::Type::getFloatTy(*irgen->GetContext()), 3);
    else if (type == Type::vec4Type) return llvm::VectorType::get(llvm::Type::getFloatTy(*irgen->GetContext()), 4);
    else return NULL;
}

llvm::Type * FnDecl::getLLVMType() const {

    if (GetType() == Type::intType) return irgen->GetIntType();
    else if (GetType() == Type::boolType) return irgen->GetBoolType();
    else if (GetType() == Type::floatType) return irgen->GetFloatType();
    else if (GetType() == Type::voidType) return llvm::Type::getVoidTy(*irgen->GetContext());
    else if (GetType() == Type::voidType) return llvm::Type::getVoidTy(*irgen->GetContext());
    else if (GetType() == Type::vec2Type) return llvm::VectorType::get(llvm::Type::getFloatTy(*irgen->GetContext()), 2);
    else if (GetType() == Type::vec3Type) return llvm::VectorType::get(llvm::Type::getFloatTy(*irgen->GetContext()), 3);
    else if (GetType() == Type::vec4Type) return llvm::VectorType::get(llvm::Type::getFloatTy(*irgen->GetContext()), 4);
    else return NULL;
}

VarDecl::VarDecl(Identifier *n, Type *t, Expr *e) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
    typeq = NULL;
}

VarDecl::VarDecl(Identifier *n, TypeQualifier *tq, Expr *e) : Decl(n) {
    Assert(n != NULL && tq != NULL);
    (typeq=tq)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
    type = NULL;
}

VarDecl::VarDecl(Identifier *n, Type *t, TypeQualifier *tq, Expr *e) : Decl(n) {
    Assert(n != NULL && t != NULL && tq != NULL);
    (type=t)->SetParent(this);
    (typeq=tq)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
}

void VarDecl::PrintChildren(int indentLevel) {
   if (typeq) typeq->Print(indentLevel+1);
   if (type) type->Print(indentLevel+1);
   if (id) id->Print(indentLevel+1);
   if (assignTo) assignTo->Print(indentLevel+1, "(initializer) ");
}

FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
    returnTypeq = NULL;
}

FnDecl::FnDecl(Identifier *n, Type *r, TypeQualifier *rq, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r != NULL && rq != NULL&& d != NULL);
    (returnType=r)->SetParent(this);
    (returnTypeq=rq)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::SetFunctionBody(Stmt *b) {
    (body=b)->SetParent(this);
}

void FnDecl::PrintChildren(int indentLevel) {
    if (returnType) returnType->Print(indentLevel+1, "(return type) ");
    if (id) id->Print(indentLevel+1);
    if (formals) formals->PrintAll(indentLevel+1, "(formals) ");
    if (body) body->Print(indentLevel+1, "(body) ");
}
