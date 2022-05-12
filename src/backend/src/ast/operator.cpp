
#include "llvm/ADT/Optional.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

#include <bits/stdc++.h>
#include "llvm_utils.h"
#include "type_utils.h"
#include "ast/operator.h"

using namespace llvm;


Value *ArraySubExprAST::codegen(bool wantPtr) {
    // std::cout<<"Array Sub"<<"Want Ptr:" << wantPtr<<std::endl;
    static Value* zero = llvmBuilder->getInt64(0);
    auto index = sub->codegen();
    if (!index) return nullptr;
    auto varPtr = var->codegen(true);
    if (!varPtr) return nullptr;
    Value* elementPtr;
    auto castIndex = createCast(index, Type::getInt64Ty(*llvmContext));
    Type * elementType = getVarType(type);
    if(varPtr->getType()->getPointerElementType()->isPointerTy()){
        // unsafe way
        auto arrayPtr = llvmBuilder->CreateLoad(elementType->getPointerTo(), varPtr);
        elementPtr = llvmBuilder->CreateGEP(
            elementType,
            arrayPtr,
            castIndex
        );
    }else{
        // safeway
        elementPtr = llvmBuilder->CreateGEP(
            ArrayType::get(elementType, varPtr->getType()->getPointerElementType()->getArrayNumElements()), 
            varPtr, 
            {zero, castIndex}
        );
    }
    if(wantPtr){
        return elementPtr;
    }
    return llvmBuilder->CreateLoad(elementType, elementPtr);
};

Value *UnaryExprAST::codegen(bool wantPtr) {
    int opType = getUnaryOpType(op);
    // print("Unary op: " + op);
    auto varType = getVarType(type);
    Value *right;    

    switch (opType) {
        case POS: {
            Value *right = rhs->codegen();
            return right;
        }
        case NEG: {
            if (right->getType()->isFloatingPointTy())
                return llvmBuilder->CreateFNeg(right);
            else 
                return llvmBuilder->CreateNeg(right);
        }
        
        case REF: {
            Value * ptr = rhs->codegen(true);          
            if (!ptr) return logErrorV("Referring to a undefined variable");
            return ptr;
        }

        case DEREF: {
            Value *right = rhs->codegen(wantPtr);
            if (!right) return logErrorV("Unable to do dereferring");
            if(wantPtr && right->getType()->getPointerElementType() == varType){
                /* 
                    this indicates right is already a ptr to char, int, double, etc...
                    use to handle the special case that *(ptr + offset) as left part
                    of an assign statement, in this case, we know that since
                    ptr + offset don't actually refer to a variable in context, it is
                    meaningless to create load from it.
                */
                return right;
            }
            if(!(right->getType()->getPointerElementType() == (wantPtr ? varType->getPointerTo() : varType))){
                /* 
                    this indicates right is already a ptr to array, since it's meaning less to assign to an array
                    variable, we'd better run codegen() without want ptr flag
                */
                right = rhs->codegen();
                return right;
            }
            auto V = llvmBuilder->CreateLoad(wantPtr ? varType->getPointerTo() : varType, right);
            if (!V) return logErrorV("Unable to do dereferring");
            return V;
        }

        case CAST: {
            Value * right = rhs->codegen();
            Value * casted = createCast(right, getVarType(type));
            if(!casted) return logErrorV("Unsupported Cast");
            return casted;
        }
        case LNOT: {
            Value * right = rhs->codegen();
            if(right->getType()->isFloatingPointTy())
                return llvmBuilder->CreateFCmpOEQ(right, getInitVal(llvmBuilder->getDoubleTy()));
            else{
                right = createCast(right, llvmBuilder->getInt64Ty());
                return llvmBuilder->CreateICmpEQ(right, llvmBuilder->getInt64(0));
            }
        }
        default: {
            auto errorMsg = "Invalid unary op" + op;
            return logErrorV(errorMsg.c_str());
        }
    }
}

Value *BinaryExprAST::codegen(bool wantPtr) {
    // Assignment
    if (op_type == ASSIGN || op_type == INC || op_type == DEC) {
        Value * variable = lhs->codegen(true);
        Value * val = rhs->codegen();
        if (!variable || !val) {
            return logErrorV("Invaild Reference");
        }
        auto castedVal = createCast(val, variable->getType()->getPointerElementType());
        if (!castedVal){
            return logErrorV((std::string("Unsupported assign from "+ getLLVMTypeStr(val->getType()) +" to " + getLLVMTypeStr(variable->getType()->getPointerElementType())).c_str()));
        }
        val = castedVal;
        Value * oldVal;
        if(op_type == INC || op_type == DEC){
            oldVal = lhs->codegen();
        }
        llvmBuilder->CreateStore(val, variable);
        return (op_type == INC || op_type == DEC) ? oldVal : castedVal;
    }

    Value *left = lhs->codegen();
    // std::cout << "Value on lhs: " + getLLVMTypeStr(left) << std::endl;
    Value *right = rhs->codegen();
    // std::cout << "Value on rhs: " + getLLVMTypeStr(right) << std::endl;
    if (!left || !right) {
        return logErrorV("lhs / rhs is not valid");
    }

    // Logical operations
    if(op_type == LAND || op_type ==  LOR){
        if(left->getType()->isFloatingPointTy()){
            left = llvmBuilder->CreateFCmpONE(left, getInitVal(llvmBuilder->getDoubleTy()));
        }else{
            left = createCast(left, llvmBuilder->getInt64Ty());
            left = llvmBuilder->CreateICmpNE(left, llvmBuilder->getInt64(0));
        }
        if(right->getType()->isFloatingPointTy()){
            right = llvmBuilder->CreateFCmpONE(right, getInitVal(llvmBuilder->getDoubleTy()));
        }else{
            right = createCast(right, llvmBuilder->getInt64Ty());
            right = llvmBuilder->CreateICmpNE(right, llvmBuilder->getInt64(0));
        }
        return op_type == LAND ? llvmBuilder->CreateLogicalAnd(left, right) : llvmBuilder->CreateLogicalOr(left, right);
    }


    // Mathematics operations
    auto leftType = left->getType();
    auto rightType = right->getType();

    if (leftType->isPointerTy() && rightType->isPointerTy()) {
        return logErrorV("Unsupported operation between pointers");
    }

    if(leftType->isPointerTy() || rightType->isPointerTy()) {
        // print("Pointer operation");
        auto newLeft = leftType->isPointerTy() ? left : right;
        auto newRight = leftType->isPointerTy() ? right : left;
        left = newLeft;
        right = newRight;
        switch(op_type) {
            case ADD:
                return llvmBuilder->CreateGEP(leftType->getPointerElementType(), left, right);
            case SUB:
                right = llvmBuilder->CreateNeg(right);
                return llvmBuilder->CreateGEP(leftType->getPointerElementType(), left, right);            
            default:
                return logErrorV("Unsupported binary operation for pointer");
        }
    }

    if(leftType->isFloatingPointTy() || rightType->isFloatingPointTy()) {
        // always cast int to FP
        auto FPleft = createCast(left, Type::getDoubleTy(*llvmContext));
        auto FPright = createCast(right, Type::getDoubleTy(*llvmContext));
        if(!FPleft || !FPright){
            return logErrorV(std::string("Unsupported operand between "+ getLLVMTypeStr(left->getType()) +" and " + getLLVMTypeStr(right->getType())).c_str());
        }else{
            left = FPleft;
            right = FPright;
        }
        switch(op_type) {
            case ADD:
                return llvmBuilder->CreateFAdd(left, right, "fadd");
            case SUB:
                return llvmBuilder->CreateFSub(left, right, "fsub");
            case MUL:
                return llvmBuilder->CreateFMul(left, right, "fmul");
            case DIV:
                return llvmBuilder->CreateFDiv(left, right, "fdiv");
            case LT:
                return llvmBuilder->CreateFCmpOLT(left, right, "flt");
            case GT:
                return llvmBuilder->CreateFCmpOGT(left, right, "fgt");
            case EQ:
                return llvmBuilder->CreateFCmpOEQ(left, right, "feq");
            case NE:
                return llvmBuilder->CreateFCmpUNE(left, right, "fne");
            case LE:
                return llvmBuilder->CreateFCmpOLE(left, right, "fle");
            case GE:
                return llvmBuilder->CreateFCmpOGE(left, right, "fge");
            default:
                return logErrorV("Invalid binary operator");
        }
    }else{
        // Otherwise, always try to convert right to left!
        right = createCast(right, left->getType());
        if(!right){
            return logErrorV((std::string("Unsupported operand between "+ getLLVMTypeStr(left->getType()) +" : " + getLLVMTypeStr(right->getType())).c_str()));
        }
        switch(op_type) {
            case ADD:
                return llvmBuilder->CreateAdd(left, right, "iadd");
            case SUB:
                return llvmBuilder->CreateSub(left, right, "isub");
            case MUL:
                return llvmBuilder->CreateMul(left, right, "imul");
            case DIV:
                return llvmBuilder->CreateSDiv(left, right, "idiv");
            case LT:
                return llvmBuilder->CreateICmpSLT(left, right, "ilt");
            case GT:
                return llvmBuilder->CreateICmpSGT(left, right, "igt");
            case LE:
                return llvmBuilder->CreateICmpSLE(left, right, "ile");
            case GE:
                return llvmBuilder->CreateICmpSGE(left, right, "ige");
            case NE:
                return llvmBuilder->CreateICmpNE(left, right, "ine");
            case EQ:
                return llvmBuilder->CreateICmpEQ(left, right, "ieq");
            case REM:
                return llvmBuilder->CreateSRem(left, right, "ieq");
            default:
                return logErrorV("Invalid binary operator");
        }
    }
}