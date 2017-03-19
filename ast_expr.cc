/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */

#include <string.h>
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "symtable.h"



llvm::Value* ArrayAccess::Emit() {
/*-----------------*/
/*
//llvm::GetElementPtrInst::Create(Value *Ptr, ArrayRef<Value*> IdxList, const Twine &NameStr, BasicBlock *InsertAtEnd);
//idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
std::vector<llvm::Value*> arrayBase;
arrayBase.push_back(llvm::ConstantInt::get(irgen->GetIntType(), 0));
arrayBase.push_back(subscript->Emit());
llvm::Value* arrayElem = llvm::GetElementPtrInst::Create(dynamic_cast<llvm::LoadInst*>(base->Emit())->getPointerOperand(), arrayBase, "", irgen->GetBasicBlock());
//llvm::Value* lInst = new llvm::LoadInst( v, exprName, irgen->GetBasicBlock() );
return new llvm::LoadInst(arrayElem, "", irgen->GetBasicBlock());
*/
/*-----------------*/

    std::vector<llvm::Value*> *theActualArray = new std::vector<llvm::Value*>();
    llvm::Value *pushing = llvm::ConstantInt::get(irgen->GetIntType(), 0);

    theActualArray->push_back(pushing);
    theActualArray->push_back(subscript->Emit());

		llvm::LoadInst *arrayPtr = dynamic_cast<llvm::LoadInst*>(base->Emit());
    llvm::Value* daElement =
        llvm::GetElementPtrInst::Create
        (arrayPtr->getPointerOperand(), *theActualArray, "", irgen->GetBasicBlock());


    return new llvm::LoadInst(daElement, "", irgen->GetBasicBlock());
}

llvm::Value* Call::Emit() {
    llvm::Function *funcFromSymtab = (llvm::Function*)symtab->find(field->GetName());
    vector<llvm::Value*> actualsList = vector<llvm::Value*>();

    for (int i = 0; i < actuals->NumElements(); ++i) {
        actualsList.push_back(actuals->Nth(i)->Emit());
    }
    llvm::Value *returning =
        llvm::CallInst::Create(funcFromSymtab, llvm::ArrayRef<llvm::Value*>(actualsList), "", irgen->GetBasicBlock());
    return returning;
}

llvm::Value* ConditionalExpr::Emit() {
    llvm::BasicBlock* block = irgen->GetBasicBlock();
    llvm::Value* tExp = trueExpr->Emit();
    llvm::Value* fExp = falseExpr->Emit();
    llvm::Value* test = cond->Emit();

    return llvm::SelectInst::Create(test, tExp, fExp, "", block);
}

llvm::Value* EqualityExpr::Emit() {
    llvm::Value *leftExpr = left->Emit();
    llvm::Value *rightExpr = right->Emit();
    llvm::BasicBlock *block = irgen->GetBasicBlock();

    /*----- INT EQUALITY -----*/
    if (leftExpr->getType() == irgen->GetIntType() &&
    rightExpr->getType() == irgen->GetIntType()) {
        if (op->IsOp("==")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::ICmp, llvm::ICmpInst::ICMP_EQ,
                leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("!=")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::ICmp, llvm::ICmpInst::ICMP_NE,
                leftExpr, rightExpr, "", block);
        }
    }

    /*----- FLOAT EQUALITY -----*/
    else if (leftExpr->getType() == irgen->GetFloatType() &&
    rightExpr->getType() == irgen->GetFloatType()) {
        if (op->IsOp("==")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::FCmp, llvm::ICmpInst::FCMP_OEQ,
                leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("!=")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::FCmp, llvm::FCmpInst::FCMP_ONE,
                leftExpr, rightExpr, "", block);
        }
    }
    return NULL;
}

llvm::Value* LogicalExpr::Emit() {
    llvm::Value *leftExpr = left->Emit();
    llvm::Value *rightExpr = right->Emit();
    llvm::BasicBlock *block = irgen->GetBasicBlock();

    if (leftExpr->getType() == irgen->GetBoolType() &&
    rightExpr->getType() == irgen->GetBoolType()) {
        if (op->IsOp("&&")) {
            return llvm::BinaryOperator::CreateAnd
                (leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("||")) {
            return llvm::BinaryOperator::CreateOr
                (leftExpr, rightExpr, "", block);
        }
    }
    return NULL;
}

llvm::Value* PostfixExpr::Emit() {
    llvm::Value *leftExpr = left->Emit();
    llvm::BasicBlock *block = irgen->GetBasicBlock();
    llvm::LoadInst *loc = llvm::cast<llvm::LoadInst>(leftExpr);

    if (leftExpr->getType() == irgen->GetIntType()) {
        if (op->IsOp("++")) {
            llvm::Value *val = llvm::BinaryOperator::CreateAdd(leftExpr,
                llvm::ConstantInt::get(irgen->GetIntType(), 1), "", block);
            new llvm::StoreInst(val, loc->getPointerOperand(), block);
            return val;
        }
        else if (op->IsOp("--")) {
            llvm::Value *val = llvm::BinaryOperator::CreateSub(leftExpr,
                llvm::ConstantInt::get(irgen->GetIntType(), 1), "", block);
            new llvm::StoreInst(val, loc->getPointerOperand(), block);
            return val;
        }
        else if (op->IsOp("+")) {
            return leftExpr;
        }
        else if (op->IsOp("-")) {
            return llvm::BinaryOperator::CreateMul(leftExpr,
                llvm::ConstantInt::get(irgen->GetIntType(), 1, true),
                "", block);
        }
    }
    else if (leftExpr->getType() == irgen->GetFloatType()) {
        if (op->IsOp("++")) {
            llvm::Value *val = llvm::BinaryOperator::CreateFAdd(leftExpr,
                llvm::ConstantFP::get(irgen->GetFloatType(), 1.0), "", block);
            new llvm::StoreInst(val, loc->getPointerOperand(), block);
            return val;
        }
        else if (op->IsOp("--")) {
            llvm::Value *val = llvm::BinaryOperator::CreateFSub(leftExpr,
                llvm::ConstantFP::get(irgen->GetFloatType(), 1.0), "", block);
            new llvm::StoreInst(val, loc->getPointerOperand(), block);
            return val;
        }
        else if (op->IsOp("+")) {
            return leftExpr;
        }
        else if (op->IsOp("-")) {
            return llvm::BinaryOperator::CreateFMul(leftExpr,
                llvm::ConstantFP::get(irgen->GetFloatType(), -1.0),
                "", block);
        }
    }
}

llvm::Value* RelationalExpr::Emit() {
    llvm::Value *leftExpr = left->Emit();
    llvm::Value *rightExpr = right->Emit();
    llvm::BasicBlock *block = irgen->GetBasicBlock();

    if (leftExpr->getType() == irgen->GetIntType() &&
    rightExpr->getType() == irgen->GetIntType()) {
        if (op->IsOp(">")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::ICmp, llvm::ICmpInst::ICMP_SGT,
                leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp(">=")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::ICmp, llvm::ICmpInst::ICMP_SGE,
                leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("<")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::ICmp, llvm::ICmpInst::ICMP_SLT,
                leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("<=")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::ICmp, llvm::ICmpInst::ICMP_SLE,
                leftExpr, rightExpr, "", block);
        }
    }

    else if (leftExpr->getType() == irgen->GetFloatType() &&
    rightExpr->getType() == irgen->GetFloatType()) {
        if (op->IsOp(">")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::FCmp, llvm::FCmpInst::FCMP_OGT,
                leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp(">=")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::FCmp, llvm::FCmpInst::FCMP_OGE,
                leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("<")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::FCmp, llvm::FCmpInst::FCMP_OLT,
                leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("<=")) {
            return llvm::CmpInst::Create
                (llvm::CmpInst::FCmp, llvm::FCmpInst::FCMP_OLE,
                leftExpr, rightExpr, "", block);
        }
    }
}

llvm::Value* AssignExpr::Emit() {
    llvm::Value *leftExpr = left->Emit();
    llvm::Value *rightExpr = right->Emit();
    llvm::BasicBlock *block = irgen->GetBasicBlock();

    /*---------- LEFT: INT, RIGHT: INT ----------*/
    if (leftExpr->getType() == irgen->GetIntType() &&
    rightExpr->getType() == irgen->GetIntType()) {

        llvm::LoadInst *leftLoc = llvm::cast<llvm::LoadInst>(leftExpr);

        if (op->IsOp("=")) {
            new llvm::StoreInst(rightExpr, leftLoc->getPointerOperand(), block);
            return rightExpr;
        }

        else if (op->IsOp("+=")) {
            llvm::Value* result = llvm::BinaryOperator::CreateAdd(leftExpr, rightExpr, "", block);
            new llvm::StoreInst(result, leftLoc->getPointerOperand(), block);
            return result;
        }

        else if (op->IsOp("-=")) {
            llvm::Value* result = llvm::BinaryOperator::CreateSub(leftExpr, rightExpr, "", block);
            new llvm::StoreInst(result, leftLoc->getPointerOperand(), block);
            return result;
        }

        else if (op->IsOp("*=")) {
            llvm::Value* result = llvm::BinaryOperator::CreateMul(leftExpr, rightExpr, "", block);
            new llvm::StoreInst(result, leftLoc->getPointerOperand(), block);
            return result;
        }

        else if (op->IsOp("/=")) {
            llvm::Value* result = llvm::BinaryOperator::CreateSDiv(leftExpr, rightExpr, "", block);
            new llvm::StoreInst(result, leftLoc->getPointerOperand(), block);
            return result;
        }
    }


    /*---------- LEFT: FLOAT, RIGHT: FLOAT ----------*/
    else if (leftExpr->getType() == irgen->GetFloatType() &&
    rightExpr->getType() == irgen->GetFloatType()) {

        llvm::LoadInst *leftLoc = llvm::cast<llvm::LoadInst>(leftExpr);

        if (op->IsOp("=")) {
            new llvm::StoreInst(rightExpr, leftLoc->getPointerOperand(), block);
            return rightExpr;
        }

        else if (op->IsOp("+=")) {
            llvm::Value* result = llvm::BinaryOperator::CreateFAdd(leftExpr, rightExpr, "", block);
            new llvm::StoreInst(result, leftLoc->getPointerOperand(), block);
            return result;
        }

        else if (op->IsOp("-=")) {
            llvm::Value* result = llvm::BinaryOperator::CreateFSub(leftExpr, rightExpr, "", block);
            new llvm::StoreInst(result, leftLoc->getPointerOperand(), block);
            return result;
        }

        else if (op->IsOp("*=")) {
            llvm::Value* result = llvm::BinaryOperator::CreateFMul(leftExpr, rightExpr, "", block);
            new llvm::StoreInst(result, leftLoc->getPointerOperand(), block);
            return result;
        }

        else if (op->IsOp("/=")) {
            llvm::Value* result = llvm::BinaryOperator::CreateFDiv(leftExpr, rightExpr, "", block);
            new llvm::StoreInst(result, leftLoc->getPointerOperand(), block);
            return result;
        }
    }
    return NULL;
}

llvm::Value* ArithmeticExpr::Emit() {
    llvm::Value *leftExpr;
    llvm::Value *rightExpr;
    llvm::BasicBlock *block = irgen->GetBasicBlock();

    if (left) {
        leftExpr = left->Emit();
    }

    if (right) {
        rightExpr = right->Emit();
    }

    /*---------- PREFIX UNARY ----------*/
    if (rightExpr && !leftExpr) {

        llvm::LoadInst *loc = llvm::cast<llvm::LoadInst>(rightExpr);

        if (rightExpr->getType() == irgen->GetIntType()) {
            if (op->IsOp("++")) {
                llvm::Value *val = llvm::BinaryOperator::CreateAdd(rightExpr,
                    llvm::ConstantInt::get(irgen->GetIntType(), 1), "", block);
                new llvm::StoreInst(val, loc->getPointerOperand(), block);
                return val;
            }
            else if (op->IsOp("--")) {
                llvm::Value *val = llvm::BinaryOperator::CreateSub(rightExpr,
                    llvm::ConstantInt::get(irgen->GetIntType(), 1), "", block);
                new llvm::StoreInst(val, loc->getPointerOperand(), block);
                return val;
            }
            else if (op->IsOp("+")) {
                return rightExpr;
            }
            else if (op->IsOp("-")) {
                return llvm::BinaryOperator::CreateMul(rightExpr,
                    llvm::ConstantInt::get(irgen->GetIntType(), 1, true),
                    "", block);
            }
        }
        else if (rightExpr->getType() == irgen->GetFloatType()) {
            if (op->IsOp("++")) {
                llvm::Value *val = llvm::BinaryOperator::CreateFAdd(rightExpr,
                    llvm::ConstantFP::get(irgen->GetFloatType(), 1.0), "", block);
                new llvm::StoreInst(val, loc->getPointerOperand(), block);
                return val;
            }
            else if (op->IsOp("--")) {
                llvm::Value *val = llvm::BinaryOperator::CreateFSub(rightExpr,
                    llvm::ConstantFP::get(irgen->GetFloatType(), 1.0), "", block);
                new llvm::StoreInst(val, loc->getPointerOperand(), block);
                return val;
            }
            else if (op->IsOp("+")) {
                return rightExpr;
            }
            else if (op->IsOp("-")) {
                return llvm::BinaryOperator::CreateFMul(rightExpr,
                    llvm::ConstantFP::get(irgen->GetFloatType(), -1.0),
                    "", block);
            }
        }
    }

    /*---------- LEFT: INT, RIGHT: INT ----------*/
    else if (leftExpr->getType() == irgen->GetIntType() &&
    rightExpr->getType() == irgen->GetIntType()) {
        if (op->IsOp("+")) {
            return llvm::BinaryOperator::CreateAdd (leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("-")) {
            return llvm::BinaryOperator::CreateSub(leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("*")) {
            return llvm::BinaryOperator::CreateMul(leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("/")) {
            return llvm::BinaryOperator::CreateSDiv(leftExpr, rightExpr, "", block);
        }
    }


    /*---------- LEFT: FLOAT, RIGHT: FLOAT ----------*/
    else if (leftExpr->getType() == irgen->GetFloatType() &&
    rightExpr->getType() == irgen->GetFloatType()) {
        if (op->IsOp("+")) {
            return llvm::BinaryOperator::CreateFAdd(leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("-")) {
            return llvm::BinaryOperator::CreateFSub(leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("*")) {
            return llvm::BinaryOperator::CreateFMul(leftExpr, rightExpr, "", block);
        }
        else if (op->IsOp("/")) {
            return llvm::BinaryOperator::CreateFDiv(leftExpr, rightExpr, "", block);
        }
    }


    return NULL;
}

llvm::Value* IntConstant::Emit() {
    return llvm::ConstantInt::get(irgen->GetIntType(), value);
}

llvm::Value* FloatConstant::Emit() {
    return llvm::ConstantFP::get(irgen->GetFloatType(), value);
}

llvm::Value* BoolConstant::Emit() {
    return llvm::ConstantInt::get(irgen->GetBoolType(), value);
}

llvm::Value* VarExpr::Emit() {
    llvm::Value *symbolValue = symtab->find(GetIdentifier()->GetName())->value;
    return new llvm::LoadInst(symbolValue, GetIdentifier()->GetName(),
        irgen->GetBasicBlock());
}

IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}
void IntConstant::PrintChildren(int indentLevel) {
    printf("%d", value);
}

FloatConstant::FloatConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}
void FloatConstant::PrintChildren(int indentLevel) {
    printf("%g", value);
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}
void BoolConstant::PrintChildren(int indentLevel) {
    printf("%s", value ? "true" : "false");
}

VarExpr::VarExpr(yyltype loc, Identifier *ident) : Expr(loc) {
    Assert(ident != NULL);
    this->id = ident;
}

void VarExpr::PrintChildren(int indentLevel) {
    id->Print(indentLevel+1);
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

void Operator::PrintChildren(int indentLevel) {
    printf("%s",tokenString);
}

bool Operator::IsOp(const char *op) const {
    return strcmp(tokenString, op) == 0;
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r)
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this);
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r)
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL;
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o)
  : Expr(Join(l->GetLocation(), o->GetLocation())) {
    Assert(l != NULL && o != NULL);
    (left=l)->SetParent(this);
    (op=o)->SetParent(this);
}

void CompoundExpr::PrintChildren(int indentLevel) {
   if (left) left->Print(indentLevel+1);
   op->Print(indentLevel+1);
   if (right) right->Print(indentLevel+1);
}

ConditionalExpr::ConditionalExpr(Expr *c, Expr *t, Expr *f)
  : Expr(Join(c->GetLocation(), f->GetLocation())) {
    Assert(c != NULL && t != NULL && f != NULL);
    (cond=c)->SetParent(this);
    (trueExpr=t)->SetParent(this);
    (falseExpr=f)->SetParent(this);
}

void ConditionalExpr::PrintChildren(int indentLevel) {
    cond->Print(indentLevel+1, "(cond) ");
    trueExpr->Print(indentLevel+1, "(true) ");
    falseExpr->Print(indentLevel+1, "(false) ");
}
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this);
    (subscript=s)->SetParent(this);
}

void ArrayAccess::PrintChildren(int indentLevel) {
    base->Print(indentLevel+1);
    subscript->Print(indentLevel+1, "(subscript) ");
}

FieldAccess::FieldAccess(Expr *b, Identifier *f)
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
}


void FieldAccess::PrintChildren(int indentLevel) {
    if (base) base->Print(indentLevel+1);
    field->Print(indentLevel+1);
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

void Call::PrintChildren(int indentLevel) {
   if (base) base->Print(indentLevel+1);
   if (field) field->Print(indentLevel+1);
   if (actuals) actuals->PrintAll(indentLevel+1, "(actuals) ");
}
