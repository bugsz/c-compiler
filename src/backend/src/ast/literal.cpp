#include <bits/stdc++.h>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"

#include "llvm/IR/Module.h"

#include "llvm_utils.h"
#include "type_utils.h"
#include "ast/literal.h"

using namespace llvm;

Value *LiteralExprAST::codegen(bool wantPtr) {
    // print("LiteralExpr, typeid: " + std::to_string(this->getType()));
    switch(this->getType()) {
        case TYPEID_INT: {
            auto intCode = ConstantInt::get(*llvmContext, APInt(32, atoi(value.c_str())));
            return intCode;
        }
        case TYPEID_DOUBLE: {
            auto doubleCode = ConstantFP::get(*llvmContext, APFloat(atof(value.c_str())));
            return doubleCode;
        }
        case TYPEID_STR: {
            std::cout << "Creating string literal: " << value << std::endl;

            auto str = this->value;
            auto charType = IntegerType::get(*llvmContext, 8);

            //1. Initialize chars vector
            std::vector<Constant *> chars(str.length());
            for(unsigned int i = 0; i < str.size(); i++) {
                chars[i] = ConstantInt::get(charType, str[i]);
            }

            //1b. add a zero terminator too
            chars.push_back(ConstantInt::get(charType, 0));

            //2. Initialize the string from the characters
            auto stringType = ArrayType::get(charType, chars.size());

            //3. Create the declaration statement
            
            auto globalDeclaration = (GlobalVariable*) llvmModule->getOrInsertGlobal(std::to_string(llvmModule->getGlobalList().size())+".str", stringType);
            globalDeclaration->setInitializer(ConstantArray::get(stringType, chars));
            globalDeclaration->setConstant(true);
            globalDeclaration->setLinkage(GlobalValue::LinkageTypes::PrivateLinkage);
            globalDeclaration->setUnnamedAddr (GlobalValue::UnnamedAddr::Global);

            //4. Return a cast to an i8*
            return ConstantExpr::getBitCast(globalDeclaration, charType->getPointerTo());
        }
        case TYPEID_CHAR: {
            auto charCode = ConstantInt::get(*llvmContext, APInt(8, value[0]));
            return charCode;
        }
        default:
            return logErrorV("Invalid type!");
    }
}