#include "yen/llvm_codegen.h"
#include <iostream>
#include <cstdlib>

#ifdef HAVE_LLVM

namespace yen {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

LLVMCodeGen::LLVMCodeGen(const std::string& moduleName, TypeChecker* tc)
    : context(std::make_unique<llvm::LLVMContext>()),
      builder(std::make_unique<llvm::IRBuilder<>>(*context)),
      module(std::make_unique<llvm::Module>(moduleName, *context)),
      typeChecker(tc),
      currentFunction(nullptr),
      debugCompileUnit(nullptr),
      debugFile(nullptr),
      debugScope(nullptr),
      sourceFilename(""),
      enableDebugInfo(false) {

    // Initialize LLVM targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // Note: The LLVM compiler is currently under development and not functional.
    // The type checker and function declarations are implemented, but the code
    // generation has critical bugs that need to be resolved.
    // For now, please use the interpreter (yen) which is fully functional.

    // Declare all native library functions
    declareNativeLibraryFunctions();
}

LLVMCodeGen::~LLVMCodeGen() = default;

// ============================================================================
// MAIN GENERATION
// ============================================================================

bool LLVMCodeGen::generate(const std::vector<std::unique_ptr<Statement>>& statements) {
    errors.clear();

    // Generate code for each statement
    for (const auto& stmt : statements) {
        codegen(stmt.get());
    }

    // Finalize debug info if enabled
    finalizeDebugInfo();

    // Verify module
    if (!verifyModule()) {
        return false;
    }

    return errors.empty();
}

// ============================================================================
// STATEMENT CODE GENERATION
// ============================================================================

void LLVMCodeGen::codegen(const Statement* stmt) {
    if (!stmt) return;

    if (auto* let = dynamic_cast<const LetStmt*>(stmt)) {
        codegenLetStmt(let);
    } else if (auto* assign = dynamic_cast<const AssignStmt*>(stmt)) {
        codegenAssignStmt(assign);
    } else if (auto* externBlk = dynamic_cast<const ExternBlock*>(stmt)) {
        codegenExternBlock(externBlk);
    } else if (auto* func = dynamic_cast<const FunctionStmt*>(stmt)) {
        codegenFunctionStmt(func);
    } else if (auto* ret = dynamic_cast<const ReturnStmt*>(stmt)) {
        codegenReturnStmt(ret);
    } else if (auto* ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        codegenIfStmt(ifStmt);
    } else if (auto* whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        codegenWhileStmt(whileStmt);
    } else if (auto* forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        codegenForStmt(forStmt);
    } else if (auto* block = dynamic_cast<const BlockStmt*>(stmt)) {
        codegenBlockStmt(block);
    } else if (auto* printStmt = dynamic_cast<const PrintStmt*>(stmt)) {
        codegenPrintStmt(printStmt);
    } else if (auto* exprStmt = dynamic_cast<const ExpressionStmt*>(stmt)) {
        codegenExprStmt(exprStmt);
    }
}

void LLVMCodeGen::codegenLetStmt(const LetStmt* stmt) {
    llvm::Value* initValue = codegen(stmt->expression.get());
    if (!initValue) {
        reportError("Failed to generate code for let statement");
        return;
    }

    // Allocate stack space for variable
    llvm::AllocaInst* alloca = builder->CreateAlloca(
        initValue->getType(),
        nullptr,
        stmt->name
    );

    // Store initial value
    builder->CreateStore(initValue, alloca);

    // Remember this variable
    namedValues[stmt->name] = alloca;
}

void LLVMCodeGen::codegenAssignStmt(const AssignStmt* stmt) {
    llvm::Value* value = codegen(stmt->expression.get());
    if (!value) {
        reportError("Failed to generate code for assignment");
        return;
    }

    llvm::Value* variable = namedValues[stmt->name];
    if (!variable) {
        reportError("Undefined variable: " + stmt->name);
        return;
    }

    builder->CreateStore(value, variable);
}

void LLVMCodeGen::codegenFunctionStmt(const FunctionStmt* stmt) {
    // Get parameter types (for now, assume int32)
    std::vector<llvm::Type*> paramTypes;
    for (size_t i = 0; i < stmt->parameters.size(); ++i) {
        paramTypes.push_back(builder->getInt32Ty());
    }

    // Create function type (int32 for now)
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        builder->getInt32Ty(),
        paramTypes,
        false // not vararg
    );

    // Create function
    llvm::Function* func = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        stmt->name,
        module.get()
    );

    // Set parameter names
    unsigned idx = 0;
    for (auto& arg : func->args()) {
        arg.setName(stmt->parameters[idx++]);
    }

    // Create entry block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entry);

    // Save previous function and named values
    auto prevFunction = currentFunction;
    auto prevNamedValues = namedValues;
    currentFunction = func;
    namedValues.clear();

    // Allocate parameters on stack and store values
    idx = 0;
    for (auto& arg : func->args()) {
        llvm::AllocaInst* alloca = builder->CreateAlloca(
            arg.getType(),
            nullptr,
            arg.getName()
        );
        builder->CreateStore(&arg, alloca);
        namedValues[std::string(arg.getName())] = alloca;
    }

    // Generate function body
    codegen(stmt->body.get());

    // If no explicit return, add return 0
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateRet(builder->getInt32(0));
    }

    // Verify function
    if (llvm::verifyFunction(*func, &llvm::errs())) {
        reportError("Function verification failed: " + stmt->name);
    }

    // Restore previous state
    currentFunction = prevFunction;
    namedValues = prevNamedValues;
}

void LLVMCodeGen::codegenExternBlock(const ExternBlock* block) {
    // Generate external function declarations
    for (const auto& funcDecl : block->functions) {
        // Parse parameter types
        std::vector<llvm::Type*> paramTypes;
        for (const auto& paramType : funcDecl->parameterTypes) {
            paramTypes.push_back(getLLVMTypeFromString(paramType));
        }

        // Parse return type
        llvm::Type* returnType = getLLVMTypeFromString(funcDecl->returnType);

        // Create function type
        llvm::FunctionType* funcType = llvm::FunctionType::get(
            returnType,
            paramTypes,
            funcDecl->isVarArg  // varargs support
        );

        // Create function declaration with external linkage
        llvm::Function* func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            funcDecl->name,
            module.get()
        );

        // Set parameter names (optional, but helps with debugging)
        size_t idx = 0;
        for (auto& arg : func->args()) {
            if (idx < funcDecl->parameters.size()) {
                arg.setName(funcDecl->parameters[idx]);
            }
            idx++;
        }
    }
}

void LLVMCodeGen::codegenReturnStmt(const ReturnStmt* stmt) {
    llvm::Value* retValue = codegen(stmt->value.get());
    if (!retValue) {
        reportError("Failed to generate code for return value");
        return;
    }

    builder->CreateRet(retValue);
}

void LLVMCodeGen::codegenIfStmt(const IfStmt* stmt) {
    llvm::Value* condValue = codegen(stmt->condition.get());
    if (!condValue) {
        reportError("Failed to generate code for if condition");
        return;
    }

    // Convert condition to bool
    condValue = builder->CreateICmpNE(
        condValue,
        llvm::ConstantInt::get(*context, llvm::APInt(1, 0)),
        "ifcond"
    );

    llvm::Function* func = builder->GetInsertBlock()->getParent();

    // Create blocks
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "then", func);
    llvm::BasicBlock* elseBB = stmt->elseBranch
        ? llvm::BasicBlock::Create(*context, "else")
        : nullptr;
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context, "ifcont");

    // Branch
    if (elseBB) {
        builder->CreateCondBr(condValue, thenBB, elseBB);
    } else {
        builder->CreateCondBr(condValue, thenBB, mergeBB);
    }

    // Emit then block
    builder->SetInsertPoint(thenBB);
    codegen(stmt->thenBranch.get());
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);
    }

    // Emit else block
    if (elseBB) {
        elseBB->insertInto(func);
        builder->SetInsertPoint(elseBB);
        codegen(stmt->elseBranch.get());
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(mergeBB);
        }
    }

    // Emit merge block
    mergeBB->insertInto(func);
    builder->SetInsertPoint(mergeBB);
}

void LLVMCodeGen::codegenWhileStmt(const WhileStmt* stmt) {
    llvm::Function* func = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*context, "whilecond", func);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(*context, "whilebody");
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(*context, "afterwhile");

    // Jump to condition
    builder->CreateBr(condBB);

    // Condition block
    builder->SetInsertPoint(condBB);
    llvm::Value* condValue = codegen(stmt->condition.get());
    if (!condValue) {
        reportError("Failed to generate code for while condition");
        return;
    }

    condValue = builder->CreateICmpNE(
        condValue,
        llvm::ConstantInt::get(*context, llvm::APInt(1, 0)),
        "whilecond"
    );

    builder->CreateCondBr(condValue, bodyBB, afterBB);

    // Body block
    bodyBB->insertInto(func);
    builder->SetInsertPoint(bodyBB);
    codegen(stmt->body.get());
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(condBB);
    }

    // After block
    afterBB->insertInto(func);
    builder->SetInsertPoint(afterBB);
}

void LLVMCodeGen::codegenForStmt(const ForStmt* stmt) {
    // TODO: Implement for loop codegen
    reportError("For loop codegen not yet implemented");
}

void LLVMCodeGen::codegenBlockStmt(const BlockStmt* stmt) {
    for (const auto& s : stmt->statements) {
        codegen(s.get());
    }
}

void LLVMCodeGen::codegenPrintStmt(const PrintStmt* stmt) {
    llvm::Value* value = codegen(stmt->expression.get());
    if (!value) {
        reportError("Failed to generate code for print statement");
        return;
    }

    // Create or get printf function
    llvm::Function* printfFunc = createPrintfFunction();

    // Determine format string based on type
    llvm::Type* valueType = value->getType();
    llvm::Value* formatStr;

    if (valueType->isIntegerTy(32)) {
        // int32
        formatStr = builder->CreateGlobalStringPtr("%d\n");
    } else if (valueType->isIntegerTy(64)) {
        // int64
        formatStr = builder->CreateGlobalStringPtr("%lld\n");
    } else if (valueType->isIntegerTy(1)) {
        // bool
        formatStr = builder->CreateGlobalStringPtr("%d\n");
    } else if (valueType->isDoubleTy()) {
        // float64
        formatStr = builder->CreateGlobalStringPtr("%f\n");
    } else if (valueType->isFloatTy()) {
        // float32
        formatStr = builder->CreateGlobalStringPtr("%f\n");
    } else if (valueType->isPointerTy()) {
        // Assume it's a string (char*)
        formatStr = builder->CreateGlobalStringPtr("%s\n");
    } else {
        reportError("Unsupported type for print statement");
        return;
    }

    builder->CreateCall(printfFunc, {formatStr, value});
}

void LLVMCodeGen::codegenExprStmt(const ExpressionStmt* stmt) {
    codegen(stmt->expression.get());
}

// ============================================================================
// EXPRESSION CODE GENERATION
// ============================================================================

llvm::Value* LLVMCodeGen::codegen(const Expression* expr) {
    if (!expr) return nullptr;

    if (auto* num = dynamic_cast<const NumberExpr*>(expr)) {
        return codegenNumberExpr(num);
    } else if (auto* lit = dynamic_cast<const LiteralExpr*>(expr)) {
        return codegenLiteralExpr(lit);
    } else if (auto* var = dynamic_cast<const VariableExpr*>(expr)) {
        return codegenVariableExpr(var);
    } else if (auto* bin = dynamic_cast<const BinaryExpr*>(expr)) {
        return codegenBinaryExpr(bin);
    } else if (auto* unary = dynamic_cast<const UnaryExpr*>(expr)) {
        return codegenUnaryExpr(unary);
    } else if (auto* call = dynamic_cast<const CallExpr*>(expr)) {
        return codegenCallExpr(call);
    } else if (auto* list = dynamic_cast<const ListExpr*>(expr)) {
        return codegenListExpr(list);
    } else if (auto* index = dynamic_cast<const IndexExpr*>(expr)) {
        return codegenIndexExpr(index);
    } else if (auto* boolExpr = dynamic_cast<const BoolExpr*>(expr)) {
        return codegenBoolExpr(boolExpr);
    }

    return nullptr;
}

llvm::Value* LLVMCodeGen::codegenNumberExpr(const NumberExpr* expr) {
    // Check if integer or float
    if (expr->value == static_cast<int>(expr->value)) {
        return builder->getInt32(static_cast<int>(expr->value));
    }
    return llvm::ConstantFP::get(*context, llvm::APFloat(expr->value));
}

llvm::Value* LLVMCodeGen::codegenLiteralExpr(const LiteralExpr* expr) {
    if (expr->value.holds_alternative<int>()) {
        return builder->getInt32(expr->value.get<int>());
    } else if (expr->value.holds_alternative<double>()) {
        return llvm::ConstantFP::get(*context, llvm::APFloat(expr->value.get<double>()));
    } else if (expr->value.holds_alternative<bool>()) {
        return builder->getInt1(expr->value.get<bool>());
    } else if (expr->value.holds_alternative<std::string>()) {
        return builder->CreateGlobalStringPtr(expr->value.get<std::string>());
    }

    return nullptr;
}

llvm::Value* LLVMCodeGen::codegenVariableExpr(const VariableExpr* expr) {
    llvm::Value* value = namedValues[expr->name];
    if (!value) {
        // Try to find it as a global variable or function
        if (llvm::GlobalVariable* global = module->getGlobalVariable(expr->name)) {
            // Load global variable
            return builder->CreateLoad(global->getValueType(), global, expr->name);
        }

        if (llvm::Function* func = module->getFunction(expr->name)) {
            // Return function pointer directly
            return func;
        }

        reportError("Undefined variable: " + expr->name);
        return nullptr;
    }

    // Check if it's an AllocaInst (local variable)
    if (llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(value)) {
        return builder->CreateLoad(
            alloca->getAllocatedType(),
            value,
            expr->name
        );
    }

    // If it's already a value (like a function), return it directly
    return value;
}

llvm::Value* LLVMCodeGen::codegenBinaryExpr(const BinaryExpr* expr) {
    llvm::Value* left = codegen(expr->left.get());
    llvm::Value* right = codegen(expr->right.get());

    if (!left || !right) {
        reportError("Failed to generate code for binary expression");
        return nullptr;
    }

    switch (expr->op) {
        case BinaryOp::Add:
            return builder->CreateAdd(left, right, "addtmp");
        case BinaryOp::Sub:
            return builder->CreateSub(left, right, "subtmp");
        case BinaryOp::Mul:
            return builder->CreateMul(left, right, "multmp");
        case BinaryOp::Div:
            return builder->CreateSDiv(left, right, "divtmp");
        case BinaryOp::Equal:
            return builder->CreateICmpEQ(left, right, "eqtmp");
        case BinaryOp::NotEqual:
            return builder->CreateICmpNE(left, right, "netmp");
        case BinaryOp::Less:
            return builder->CreateICmpSLT(left, right, "lttmp");
        case BinaryOp::LessEqual:
            return builder->CreateICmpSLE(left, right, "letmp");
        case BinaryOp::Greater:
            return builder->CreateICmpSGT(left, right, "gttmp");
        case BinaryOp::GreaterEqual:
            return builder->CreateICmpSGE(left, right, "getmp");
        case BinaryOp::And:
            return builder->CreateAnd(left, right, "andtmp");
        case BinaryOp::Or:
            return builder->CreateOr(left, right, "ortmp");
        default:
            reportError("Unknown binary operator");
            return nullptr;
    }
}

llvm::Value* LLVMCodeGen::codegenUnaryExpr(const UnaryExpr* expr) {
    llvm::Value* right = codegen(expr->right.get());
    if (!right) {
        reportError("Failed to generate code for unary expression");
        return nullptr;
    }

    switch (expr->op) {
        case UnaryOp::Not:
            return builder->CreateNot(right, "nottmp");
        default:
            reportError("Unknown unary operator");
            return nullptr;
    }
}

llvm::Value* LLVMCodeGen::codegenCallExpr(const CallExpr* expr) {
    // Get the function name from the callee
    std::string funcName;
    if (auto* varExpr = dynamic_cast<const VariableExpr*>(expr->callee.get())) {
        funcName = varExpr->name;
    } else {
        reportError("Call expression must have a simple function name");
        return nullptr;
    }

    // Look up the function in the module
    llvm::Function* calleeFunc = module->getFunction(funcName);
    if (!calleeFunc) {
        reportError("Undefined function: " + funcName);
        return nullptr;
    }

    // Check argument count
    if (calleeFunc->isVarArg()) {
        // Varargs function: must have at least the required arguments
        if (expr->arguments.size() < calleeFunc->arg_size()) {
            reportError("Function " + funcName + " expects at least " +
                       std::to_string(calleeFunc->arg_size()) + " arguments but got " +
                       std::to_string(expr->arguments.size()));
            return nullptr;
        }
    } else {
        // Non-varargs: must match exactly
        if (calleeFunc->arg_size() != expr->arguments.size()) {
            reportError("Incorrect number of arguments for function: " + funcName);
            return nullptr;
        }
    }

    // Generate code for arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : expr->arguments) {
        llvm::Value* argValue = codegen(arg.get());
        if (!argValue) {
            reportError("Failed to generate code for argument");
            return nullptr;
        }
        args.push_back(argValue);
    }

    // Create the call instruction
    // Don't give a name to void function calls
    if (calleeFunc->getReturnType()->isVoidTy()) {
        builder->CreateCall(calleeFunc, args);
        // Return nullptr for void calls - caller should handle this
        return nullptr;
    } else {
        return builder->CreateCall(calleeFunc, args, "calltmp");
    }
}

llvm::Value* LLVMCodeGen::codegenListExpr(const ListExpr* expr) {
    // TODO: Implement list expression codegen
    reportError("List expression codegen not yet implemented");
    return nullptr;
}

llvm::Value* LLVMCodeGen::codegenIndexExpr(const IndexExpr* expr) {
    // TODO: Implement index expression codegen
    reportError("Index expression codegen not yet implemented");
    return nullptr;
}

llvm::Value* LLVMCodeGen::codegenBoolExpr(const BoolExpr* expr) {
    return builder->getInt1(expr->value);
}

// ============================================================================
// TYPE CONVERSION
// ============================================================================

llvm::Type* LLVMCodeGen::getLLVMType(const TypePtr& yenType) {
    if (auto* prim = dynamic_cast<const PrimitiveTypeNode*>(yenType.get())) {
        switch (prim->primitiveType) {
            case PrimitiveType::Int8:
            case PrimitiveType::UInt8:
                return builder->getInt8Ty();
            case PrimitiveType::Int16:
            case PrimitiveType::UInt16:
                return builder->getInt16Ty();
            case PrimitiveType::Int32:
            case PrimitiveType::UInt32:
                return builder->getInt32Ty();
            case PrimitiveType::Int64:
            case PrimitiveType::UInt64:
                return builder->getInt64Ty();
            case PrimitiveType::Float32:
                return builder->getFloatTy();
            case PrimitiveType::Float64:
                return builder->getDoubleTy();
            case PrimitiveType::Bool:
                return builder->getInt1Ty();
            case PrimitiveType::Char:
                return builder->getInt8Ty();
            case PrimitiveType::String:
                return builder->getInt8PtrTy();
            case PrimitiveType::Void:
                return builder->getVoidTy();
            default:
                return builder->getInt32Ty();
        }
    }

    // Default to int32
    return builder->getInt32Ty();
}

llvm::Type* LLVMCodeGen::getLLVMTypeFromString(const std::string& typeStr) {
    // Handle pointer types
    if (typeStr.find("*") != std::string::npos) {
        // Extract base type (e.g., "*const char" -> "char", "*mut void" -> "void")
        std::string baseType;
        if (typeStr.find("*const ") == 0) {
            baseType = typeStr.substr(7); // Skip "*const "
        } else if (typeStr.find("*mut ") == 0) {
            baseType = typeStr.substr(5); // Skip "*mut "
        } else if (typeStr[0] == '*') {
            baseType = typeStr.substr(1); // Skip "*"
        }

        // Get pointee type
        llvm::Type* pointeeType;
        if (baseType == "void") {
            pointeeType = builder->getInt8Ty(); // void* = i8*
        } else if (baseType == "char") {
            pointeeType = builder->getInt8Ty();
        } else {
            pointeeType = getLLVMTypeFromString(baseType);
        }

        return llvm::PointerType::getUnqual(pointeeType);
    }

    // Primitive types
    if (typeStr == "int8") return builder->getInt8Ty();
    if (typeStr == "int16") return builder->getInt16Ty();
    if (typeStr == "int32" || typeStr == "int") return builder->getInt32Ty();
    if (typeStr == "int64") return builder->getInt64Ty();
    if (typeStr == "uint8") return builder->getInt8Ty();
    if (typeStr == "uint16") return builder->getInt16Ty();
    if (typeStr == "uint32") return builder->getInt32Ty();
    if (typeStr == "uint64") return builder->getInt64Ty();
    if (typeStr == "float32" || typeStr == "float") return builder->getFloatTy();
    if (typeStr == "float64") return builder->getDoubleTy();
    if (typeStr == "bool") return builder->getInt1Ty();
    if (typeStr == "char") return builder->getInt8Ty();
    if (typeStr == "string" || typeStr == "str") return builder->getInt8PtrTy();
    if (typeStr == "void") return builder->getVoidTy();

    // Opaque type names (for FILE, etc.)
    if (typeStr == "FILE") {
        // FILE is opaque in C, represented as i8* (void*)
        return builder->getInt8PtrTy();
    }

    // Default to int32
    return builder->getInt32Ty();
}

// ============================================================================
// OUTPUT GENERATION
// ============================================================================

void LLVMCodeGen::emitLLVMIR(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
        reportError("Could not open file: " + EC.message());
        return;
    }

    module->print(dest, nullptr);
    dest.flush();
}

void LLVMCodeGen::emitObjectFile(const std::string& filename) {
    // Get the target triple for the current system
    std::string targetTriple = module->getTargetTriple();
    if (targetTriple.empty()) {
        targetTriple = llvm::sys::getProcessTriple();
        module->setTargetTriple(targetTriple);
    }

    // Look up the target
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        reportError("Failed to lookup target: " + error);
        return;
    }

    // Create target machine
    llvm::TargetOptions opt;
    llvm::TargetMachine* targetMachine = target->createTargetMachine(
        targetTriple,
        "generic",
        "",
        opt,
        llvm::Reloc::PIC_
    );

    if (!targetMachine) {
        reportError("Failed to create target machine");
        return;
    }

    // Set the data layout
    module->setDataLayout(targetMachine->createDataLayout());

    // Open output file
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
    if (ec) {
        reportError("Could not open file: " + ec.message());
        delete targetMachine;
        return;
    }

    // Create pass manager and emit object file
    llvm::legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
        reportError("TargetMachine can't emit object file");
        delete targetMachine;
        return;
    }

    pass.run(*module);
    dest.flush();
    delete targetMachine;
}

void LLVMCodeGen::emitExecutable(const std::string& filename) {
    // First, emit object file to temporary location
    std::string objFile = filename + ".o";
    emitObjectFile(objFile);

    if (!errors.empty()) {
        return; // Object file emission failed
    }

    // Link with system linker (use clang for convenience)
    std::string linkCmd = "clang++ -o \"" + filename + "\" \"" + objFile + "\"";

    int result = system(linkCmd.c_str());
    if (result != 0) {
        reportError("Linking failed with exit code " + std::to_string(result));
        return;
    }

    // Clean up temporary object file
    std::remove(objFile.c_str());
}

void LLVMCodeGen::optimize(int level) {
    // TODO: Implement optimization passes
    (void)level;
}

void LLVMCodeGen::printModule() {
    module->print(llvm::outs(), nullptr);
}

bool LLVMCodeGen::verifyModule() {
    std::string errorStr;
    llvm::raw_string_ostream errorStream(errorStr);

    if (llvm::verifyModule(*module, &errorStream)) {
        reportError("Module verification failed:\n" + errorStr);
        return false;
    }

    return true;
}

// ============================================================================
// DEBUG INFORMATION
// ============================================================================

void LLVMCodeGen::setSourceFile(const std::string& filename) {
    sourceFilename = filename;
}

void LLVMCodeGen::setDebugInfo(bool enable) {
    enableDebugInfo = enable;

    if (!enableDebugInfo) {
        return;
    }

    // Create debug info builder
    debugBuilder = std::make_unique<llvm::DIBuilder>(*module);

    // Create compile unit
    // Get just the filename from the path
    std::string filename = sourceFilename;
    std::string directory = ".";

    size_t lastSlash = sourceFilename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = sourceFilename.substr(lastSlash + 1);
        directory = sourceFilename.substr(0, lastSlash);
    }

    debugFile = debugBuilder->createFile(filename, directory);

    debugCompileUnit = debugBuilder->createCompileUnit(
        llvm::dwarf::DW_LANG_C,  // Use C language tag (no specific YEN tag yet)
        debugFile,
        "YEN Compiler v0.1",     // Producer
        false,                    // isOptimized
        "",                       // Flags
        0                         // Runtime version
    );

    debugScope = debugCompileUnit;

    // Add debug info to module
    module->addModuleFlag(llvm::Module::Warning, "Debug Info Version",
                          llvm::DEBUG_METADATA_VERSION);

    // Darwin/macOS specific: use DWARF version 4
    module->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 4);
}

void LLVMCodeGen::finalizeDebugInfo() {
    if (enableDebugInfo && debugBuilder) {
        debugBuilder->finalize();
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

llvm::Function* LLVMCodeGen::createPrintfFunction() {
    llvm::Function* printfFunc = module->getFunction("printf");
    if (printfFunc) {
        return printfFunc;
    }

    // Create printf prototype: int printf(const char*, ...)
    llvm::FunctionType* printfType = llvm::FunctionType::get(
        builder->getInt32Ty(),
        {builder->getInt8PtrTy()},
        true // vararg
    );

    printfFunc = llvm::Function::Create(
        printfType,
        llvm::Function::ExternalLinkage,
        "printf",
        module.get()
    );

    return printfFunc;
}

llvm::Function* LLVMCodeGen::createMallocFunction() {
    // TODO: Implement malloc function creation
    return nullptr;
}

llvm::Function* LLVMCodeGen::createFreeFunction() {
    // TODO: Implement free function creation
    return nullptr;
}

// ============================================================================
// NATIVE LIBRARY FUNCTION DECLARATIONS
// ============================================================================

void LLVMCodeGen::declareNativeLibraryFunctions() {
    // Helper lambda to declare a function
    auto declareFunc = [this](const std::string& name, llvm::Type* retType, const std::vector<llvm::Type*>& params, bool vararg = false) {
        llvm::FunctionType* funcType = llvm::FunctionType::get(retType, params, vararg);
        llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, module.get());
    };

    // Common types
    llvm::Type* voidTy = builder->getVoidTy();
    llvm::Type* i32Ty = builder->getInt32Ty();
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* doubleTy = builder->getDoubleTy();
    llvm::Type* i8PtrTy = builder->getInt8PtrTy();
    llvm::Type* boolTy = builder->getInt1Ty();

    // ========== CORE LIBRARY ==========
    declareFunc("core_is_int", boolTy, {i8PtrTy});
    declareFunc("core_is_float", boolTy, {i8PtrTy});
    declareFunc("core_is_bool", boolTy, {i8PtrTy});
    declareFunc("core_is_string", boolTy, {i8PtrTy});
    declareFunc("core_is_list", boolTy, {i8PtrTy});
    declareFunc("core_to_int", i32Ty, {i8PtrTy});
    declareFunc("core_to_float", doubleTy, {i8PtrTy});
    declareFunc("core_to_string", i8PtrTy, {i8PtrTy});

    // ========== MATH LIBRARY ==========
    declareFunc("math_abs", doubleTy, {doubleTy});
    declareFunc("math_sqrt", doubleTy, {doubleTy});
    declareFunc("math_pow", doubleTy, {doubleTy, doubleTy});
    declareFunc("math_sin", doubleTy, {doubleTy});
    declareFunc("math_cos", doubleTy, {doubleTy});
    declareFunc("math_tan", doubleTy, {doubleTy});
    declareFunc("math_floor", doubleTy, {doubleTy});
    declareFunc("math_ceil", doubleTy, {doubleTy});
    declareFunc("math_round", doubleTy, {doubleTy});
    declareFunc("math_random", doubleTy, {});

    // Math constants (as external variables)
    new llvm::GlobalVariable(
        *module,
        doubleTy,
        true,  // isConstant
        llvm::GlobalValue::ExternalLinkage,
        nullptr,  // No initializer (external)
        "math_PI"
    );
    new llvm::GlobalVariable(
        *module,
        doubleTy,
        true,  // isConstant
        llvm::GlobalValue::ExternalLinkage,
        nullptr,  // No initializer (external)
        "math_E"
    );

    // ========== STRING LIBRARY ==========
    declareFunc("str_length", i32Ty, {i8PtrTy});
    declareFunc("str_upper", i8PtrTy, {i8PtrTy});
    declareFunc("str_lower", i8PtrTy, {i8PtrTy});
    declareFunc("str_trim", i8PtrTy, {i8PtrTy});
    declareFunc("str_split", i8PtrTy, {i8PtrTy, i8PtrTy});  // Returns array as pointer
    declareFunc("str_join", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("str_substring", i8PtrTy, {i8PtrTy, i32Ty, i32Ty});
    declareFunc("str_contains", boolTy, {i8PtrTy, i8PtrTy});
    declareFunc("str_replace", i8PtrTy, {i8PtrTy, i8PtrTy, i8PtrTy});

    // ========== COLLECTIONS LIBRARY ==========
    declareFunc("list_push", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("list_pop", i8PtrTy, {i8PtrTy});
    declareFunc("list_length", i32Ty, {i8PtrTy});
    declareFunc("list_reverse", i8PtrTy, {i8PtrTy});
    declareFunc("list_sort", i8PtrTy, {i8PtrTy});

    // ========== IO LIBRARY ==========
    declareFunc("io_read_file", i8PtrTy, {i8PtrTy});
    declareFunc("io_write_file", boolTy, {i8PtrTy, i8PtrTy});
    declareFunc("io_append_file", boolTy, {i8PtrTy, i8PtrTy});

    // ========== FS LIBRARY ==========
    declareFunc("fs_exists", boolTy, {i8PtrTy});
    declareFunc("fs_is_directory", boolTy, {i8PtrTy});
    declareFunc("fs_is_file", boolTy, {i8PtrTy});
    declareFunc("fs_create_dir", boolTy, {i8PtrTy});
    declareFunc("fs_remove", boolTy, {i8PtrTy});
    declareFunc("fs_file_size", i32Ty, {i8PtrTy});

    // ========== TIME LIBRARY ==========
    declareFunc("time_now", i64Ty, {});
    declareFunc("time_sleep", voidTy, {i32Ty});

    // ========== CRYPTO LIBRARY ==========
    declareFunc("crypto_xor", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("crypto_hash", i8PtrTy, {i8PtrTy});
    declareFunc("crypto_random_bytes", i8PtrTy, {i32Ty});

    // ========== ENCODING LIBRARY ==========
    declareFunc("encoding_base64_encode", i8PtrTy, {i8PtrTy});
    declareFunc("encoding_hex_encode", i8PtrTy, {i8PtrTy});

    // ========== LOG LIBRARY ==========
    declareFunc("log_info", voidTy, {i8PtrTy});
    declareFunc("log_warn", voidTy, {i8PtrTy});
    declareFunc("log_error", voidTy, {i8PtrTy});

    // ========== ENV LIBRARY ==========
    declareFunc("env_get", i8PtrTy, {i8PtrTy});
    declareFunc("env_set", voidTy, {i8PtrTy, i8PtrTy});

    // ========== PROCESS LIBRARY ==========
    declareFunc("process_exec", i32Ty, {i8PtrTy});
    declareFunc("process_shell", i8PtrTy, {i8PtrTy});
    declareFunc("process_spawn", i32Ty, {i8PtrTy}, true);  // varargs
    declareFunc("process_cwd", i8PtrTy, {});
    declareFunc("process_chdir", i32Ty, {i8PtrTy});

    // ========== PRINT FUNCTION ==========
    declareFunc("print", voidTy, {i8PtrTy});
}

void LLVMCodeGen::reportError(const std::string& message) {
    errors.push_back("Codegen error: " + message);
    std::cerr << "Codegen error: " << message << std::endl;
}

} // namespace yen

#endif // HAVE_LLVM
