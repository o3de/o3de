// Copyright (c) Contributors to the Open 3D Engine Project.
// For complete copyright and license terms please see the LICENSE at the root of this distribution.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT
//

// Generated from azslParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "azslParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by azslParser.
 */
class  azslParserListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterCompilationUnit(azslParser::CompilationUnitContext *ctx) = 0;
  virtual void exitCompilationUnit(azslParser::CompilationUnitContext *ctx) = 0;

  virtual void enterTopLevelDeclaration(azslParser::TopLevelDeclarationContext *ctx) = 0;
  virtual void exitTopLevelDeclaration(azslParser::TopLevelDeclarationContext *ctx) = 0;

  virtual void enterIdExpression(azslParser::IdExpressionContext *ctx) = 0;
  virtual void exitIdExpression(azslParser::IdExpressionContext *ctx) = 0;

  virtual void enterUnqualifiedId(azslParser::UnqualifiedIdContext *ctx) = 0;
  virtual void exitUnqualifiedId(azslParser::UnqualifiedIdContext *ctx) = 0;

  virtual void enterQualifiedId(azslParser::QualifiedIdContext *ctx) = 0;
  virtual void exitQualifiedId(azslParser::QualifiedIdContext *ctx) = 0;

  virtual void enterNestedNameSpecifier(azslParser::NestedNameSpecifierContext *ctx) = 0;
  virtual void exitNestedNameSpecifier(azslParser::NestedNameSpecifierContext *ctx) = 0;

  virtual void enterClassDefinitionStatement(azslParser::ClassDefinitionStatementContext *ctx) = 0;
  virtual void exitClassDefinitionStatement(azslParser::ClassDefinitionStatementContext *ctx) = 0;

  virtual void enterClassDefinition(azslParser::ClassDefinitionContext *ctx) = 0;
  virtual void exitClassDefinition(azslParser::ClassDefinitionContext *ctx) = 0;

  virtual void enterBaseList(azslParser::BaseListContext *ctx) = 0;
  virtual void exitBaseList(azslParser::BaseListContext *ctx) = 0;

  virtual void enterClassMemberDeclaration(azslParser::ClassMemberDeclarationContext *ctx) = 0;
  virtual void exitClassMemberDeclaration(azslParser::ClassMemberDeclarationContext *ctx) = 0;

  virtual void enterStructDefinitionStatement(azslParser::StructDefinitionStatementContext *ctx) = 0;
  virtual void exitStructDefinitionStatement(azslParser::StructDefinitionStatementContext *ctx) = 0;

  virtual void enterStructDefinition(azslParser::StructDefinitionContext *ctx) = 0;
  virtual void exitStructDefinition(azslParser::StructDefinitionContext *ctx) = 0;

  virtual void enterStructMemberDeclaration(azslParser::StructMemberDeclarationContext *ctx) = 0;
  virtual void exitStructMemberDeclaration(azslParser::StructMemberDeclarationContext *ctx) = 0;

  virtual void enterAnyStructuredTypeDefinitionStatement(azslParser::AnyStructuredTypeDefinitionStatementContext *ctx) = 0;
  virtual void exitAnyStructuredTypeDefinitionStatement(azslParser::AnyStructuredTypeDefinitionStatementContext *ctx) = 0;

  virtual void enterEnumDefinitionStatement(azslParser::EnumDefinitionStatementContext *ctx) = 0;
  virtual void exitEnumDefinitionStatement(azslParser::EnumDefinitionStatementContext *ctx) = 0;

  virtual void enterEnumDefinition(azslParser::EnumDefinitionContext *ctx) = 0;
  virtual void exitEnumDefinition(azslParser::EnumDefinitionContext *ctx) = 0;

  virtual void enterUnscopedEnum(azslParser::UnscopedEnumContext *ctx) = 0;
  virtual void exitUnscopedEnum(azslParser::UnscopedEnumContext *ctx) = 0;

  virtual void enterScopedEnum(azslParser::ScopedEnumContext *ctx) = 0;
  virtual void exitScopedEnum(azslParser::ScopedEnumContext *ctx) = 0;

  virtual void enterEnumeratorListDefinition(azslParser::EnumeratorListDefinitionContext *ctx) = 0;
  virtual void exitEnumeratorListDefinition(azslParser::EnumeratorListDefinitionContext *ctx) = 0;

  virtual void enterEnumeratorDeclarator(azslParser::EnumeratorDeclaratorContext *ctx) = 0;
  virtual void exitEnumeratorDeclarator(azslParser::EnumeratorDeclaratorContext *ctx) = 0;

  virtual void enterAnyStructuredTypeDefinition(azslParser::AnyStructuredTypeDefinitionContext *ctx) = 0;
  virtual void exitAnyStructuredTypeDefinition(azslParser::AnyStructuredTypeDefinitionContext *ctx) = 0;

  virtual void enterInterfaceDefinitionStatement(azslParser::InterfaceDefinitionStatementContext *ctx) = 0;
  virtual void exitInterfaceDefinitionStatement(azslParser::InterfaceDefinitionStatementContext *ctx) = 0;

  virtual void enterInterfaceDefinition(azslParser::InterfaceDefinitionContext *ctx) = 0;
  virtual void exitInterfaceDefinition(azslParser::InterfaceDefinitionContext *ctx) = 0;

  virtual void enterInterfaceMemberDeclaration(azslParser::InterfaceMemberDeclarationContext *ctx) = 0;
  virtual void exitInterfaceMemberDeclaration(azslParser::InterfaceMemberDeclarationContext *ctx) = 0;

  virtual void enterConstantBufferTemplated(azslParser::ConstantBufferTemplatedContext *ctx) = 0;
  virtual void exitConstantBufferTemplated(azslParser::ConstantBufferTemplatedContext *ctx) = 0;

  virtual void enterVariableDeclarationStatement(azslParser::VariableDeclarationStatementContext *ctx) = 0;
  virtual void exitVariableDeclarationStatement(azslParser::VariableDeclarationStatementContext *ctx) = 0;

  virtual void enterFunctionParams(azslParser::FunctionParamsContext *ctx) = 0;
  virtual void exitFunctionParams(azslParser::FunctionParamsContext *ctx) = 0;

  virtual void enterFunctionParam(azslParser::FunctionParamContext *ctx) = 0;
  virtual void exitFunctionParam(azslParser::FunctionParamContext *ctx) = 0;

  virtual void enterHlslSemantic(azslParser::HlslSemanticContext *ctx) = 0;
  virtual void exitHlslSemantic(azslParser::HlslSemanticContext *ctx) = 0;

  virtual void enterHlslSemanticName(azslParser::HlslSemanticNameContext *ctx) = 0;
  virtual void exitHlslSemanticName(azslParser::HlslSemanticNameContext *ctx) = 0;

  virtual void enterAttributeArguments(azslParser::AttributeArgumentsContext *ctx) = 0;
  virtual void exitAttributeArguments(azslParser::AttributeArgumentsContext *ctx) = 0;

  virtual void enterAttributeArgumentList(azslParser::AttributeArgumentListContext *ctx) = 0;
  virtual void exitAttributeArgumentList(azslParser::AttributeArgumentListContext *ctx) = 0;

  virtual void enterGlobalAttribute(azslParser::GlobalAttributeContext *ctx) = 0;
  virtual void exitGlobalAttribute(azslParser::GlobalAttributeContext *ctx) = 0;

  virtual void enterAttachedAttribute(azslParser::AttachedAttributeContext *ctx) = 0;
  virtual void exitAttachedAttribute(azslParser::AttachedAttributeContext *ctx) = 0;

  virtual void enterAttributeSpecifier(azslParser::AttributeSpecifierContext *ctx) = 0;
  virtual void exitAttributeSpecifier(azslParser::AttributeSpecifierContext *ctx) = 0;

  virtual void enterAttributeSpecifierSequence(azslParser::AttributeSpecifierSequenceContext *ctx) = 0;
  virtual void exitAttributeSpecifierSequence(azslParser::AttributeSpecifierSequenceContext *ctx) = 0;

  virtual void enterAttributeSpecifierAny(azslParser::AttributeSpecifierAnyContext *ctx) = 0;
  virtual void exitAttributeSpecifierAny(azslParser::AttributeSpecifierAnyContext *ctx) = 0;

  virtual void enterBlock(azslParser::BlockContext *ctx) = 0;
  virtual void exitBlock(azslParser::BlockContext *ctx) = 0;

  virtual void enterStatement(azslParser::StatementContext *ctx) = 0;
  virtual void exitStatement(azslParser::StatementContext *ctx) = 0;

  virtual void enterForInitializer(azslParser::ForInitializerContext *ctx) = 0;
  virtual void exitForInitializer(azslParser::ForInitializerContext *ctx) = 0;

  virtual void enterCaseSwitchLabel(azslParser::CaseSwitchLabelContext *ctx) = 0;
  virtual void exitCaseSwitchLabel(azslParser::CaseSwitchLabelContext *ctx) = 0;

  virtual void enterDefaultSwitchLabel(azslParser::DefaultSwitchLabelContext *ctx) = 0;
  virtual void exitDefaultSwitchLabel(azslParser::DefaultSwitchLabelContext *ctx) = 0;

  virtual void enterSwitchSection(azslParser::SwitchSectionContext *ctx) = 0;
  virtual void exitSwitchSection(azslParser::SwitchSectionContext *ctx) = 0;

  virtual void enterSwitchBlock(azslParser::SwitchBlockContext *ctx) = 0;
  virtual void exitSwitchBlock(azslParser::SwitchBlockContext *ctx) = 0;

  virtual void enterEmptyStatement(azslParser::EmptyStatementContext *ctx) = 0;
  virtual void exitEmptyStatement(azslParser::EmptyStatementContext *ctx) = 0;

  virtual void enterBlockStatement(azslParser::BlockStatementContext *ctx) = 0;
  virtual void exitBlockStatement(azslParser::BlockStatementContext *ctx) = 0;

  virtual void enterExpressionStatement(azslParser::ExpressionStatementContext *ctx) = 0;
  virtual void exitExpressionStatement(azslParser::ExpressionStatementContext *ctx) = 0;

  virtual void enterIfStatement(azslParser::IfStatementContext *ctx) = 0;
  virtual void exitIfStatement(azslParser::IfStatementContext *ctx) = 0;

  virtual void enterSwitchStatement(azslParser::SwitchStatementContext *ctx) = 0;
  virtual void exitSwitchStatement(azslParser::SwitchStatementContext *ctx) = 0;

  virtual void enterWhileStatement(azslParser::WhileStatementContext *ctx) = 0;
  virtual void exitWhileStatement(azslParser::WhileStatementContext *ctx) = 0;

  virtual void enterDoStatement(azslParser::DoStatementContext *ctx) = 0;
  virtual void exitDoStatement(azslParser::DoStatementContext *ctx) = 0;

  virtual void enterForStatement(azslParser::ForStatementContext *ctx) = 0;
  virtual void exitForStatement(azslParser::ForStatementContext *ctx) = 0;

  virtual void enterBreakStatement(azslParser::BreakStatementContext *ctx) = 0;
  virtual void exitBreakStatement(azslParser::BreakStatementContext *ctx) = 0;

  virtual void enterContinueStatement(azslParser::ContinueStatementContext *ctx) = 0;
  virtual void exitContinueStatement(azslParser::ContinueStatementContext *ctx) = 0;

  virtual void enterDiscardStatement(azslParser::DiscardStatementContext *ctx) = 0;
  virtual void exitDiscardStatement(azslParser::DiscardStatementContext *ctx) = 0;

  virtual void enterReturnStatement(azslParser::ReturnStatementContext *ctx) = 0;
  virtual void exitReturnStatement(azslParser::ReturnStatementContext *ctx) = 0;

  virtual void enterExtenstionStatement(azslParser::ExtenstionStatementContext *ctx) = 0;
  virtual void exitExtenstionStatement(azslParser::ExtenstionStatementContext *ctx) = 0;

  virtual void enterTypeAliasingDefinitionStatementLabel(azslParser::TypeAliasingDefinitionStatementLabelContext *ctx) = 0;
  virtual void exitTypeAliasingDefinitionStatementLabel(azslParser::TypeAliasingDefinitionStatementLabelContext *ctx) = 0;

  virtual void enterElseClause(azslParser::ElseClauseContext *ctx) = 0;
  virtual void exitElseClause(azslParser::ElseClauseContext *ctx) = 0;

  virtual void enterParenthesizedExpression(azslParser::ParenthesizedExpressionContext *ctx) = 0;
  virtual void exitParenthesizedExpression(azslParser::ParenthesizedExpressionContext *ctx) = 0;

  virtual void enterMemberAccessExpression(azslParser::MemberAccessExpressionContext *ctx) = 0;
  virtual void exitMemberAccessExpression(azslParser::MemberAccessExpressionContext *ctx) = 0;

  virtual void enterPrefixUnaryExpression(azslParser::PrefixUnaryExpressionContext *ctx) = 0;
  virtual void exitPrefixUnaryExpression(azslParser::PrefixUnaryExpressionContext *ctx) = 0;

  virtual void enterLiteralExpression(azslParser::LiteralExpressionContext *ctx) = 0;
  virtual void exitLiteralExpression(azslParser::LiteralExpressionContext *ctx) = 0;

  virtual void enterConditionalExpression(azslParser::ConditionalExpressionContext *ctx) = 0;
  virtual void exitConditionalExpression(azslParser::ConditionalExpressionContext *ctx) = 0;

  virtual void enterPostfixUnaryExpression(azslParser::PostfixUnaryExpressionContext *ctx) = 0;
  virtual void exitPostfixUnaryExpression(azslParser::PostfixUnaryExpressionContext *ctx) = 0;

  virtual void enterNumericConstructorExpression(azslParser::NumericConstructorExpressionContext *ctx) = 0;
  virtual void exitNumericConstructorExpression(azslParser::NumericConstructorExpressionContext *ctx) = 0;

  virtual void enterFunctionCallExpression(azslParser::FunctionCallExpressionContext *ctx) = 0;
  virtual void exitFunctionCallExpression(azslParser::FunctionCallExpressionContext *ctx) = 0;

  virtual void enterIdentifierExpression(azslParser::IdentifierExpressionContext *ctx) = 0;
  virtual void exitIdentifierExpression(azslParser::IdentifierExpressionContext *ctx) = 0;

  virtual void enterBinaryExpression(azslParser::BinaryExpressionContext *ctx) = 0;
  virtual void exitBinaryExpression(azslParser::BinaryExpressionContext *ctx) = 0;

  virtual void enterAssignmentExpression(azslParser::AssignmentExpressionContext *ctx) = 0;
  virtual void exitAssignmentExpression(azslParser::AssignmentExpressionContext *ctx) = 0;

  virtual void enterCastExpression(azslParser::CastExpressionContext *ctx) = 0;
  virtual void exitCastExpression(azslParser::CastExpressionContext *ctx) = 0;

  virtual void enterArrayAccessExpression(azslParser::ArrayAccessExpressionContext *ctx) = 0;
  virtual void exitArrayAccessExpression(azslParser::ArrayAccessExpressionContext *ctx) = 0;

  virtual void enterOtherExpression(azslParser::OtherExpressionContext *ctx) = 0;
  virtual void exitOtherExpression(azslParser::OtherExpressionContext *ctx) = 0;

  virtual void enterCommaExpression(azslParser::CommaExpressionContext *ctx) = 0;
  virtual void exitCommaExpression(azslParser::CommaExpressionContext *ctx) = 0;

  virtual void enterPostfixUnaryOperator(azslParser::PostfixUnaryOperatorContext *ctx) = 0;
  virtual void exitPostfixUnaryOperator(azslParser::PostfixUnaryOperatorContext *ctx) = 0;

  virtual void enterPrefixUnaryOperator(azslParser::PrefixUnaryOperatorContext *ctx) = 0;
  virtual void exitPrefixUnaryOperator(azslParser::PrefixUnaryOperatorContext *ctx) = 0;

  virtual void enterBinaryOperator(azslParser::BinaryOperatorContext *ctx) = 0;
  virtual void exitBinaryOperator(azslParser::BinaryOperatorContext *ctx) = 0;

  virtual void enterAssignmentOperator(azslParser::AssignmentOperatorContext *ctx) = 0;
  virtual void exitAssignmentOperator(azslParser::AssignmentOperatorContext *ctx) = 0;

  virtual void enterArgumentList(azslParser::ArgumentListContext *ctx) = 0;
  virtual void exitArgumentList(azslParser::ArgumentListContext *ctx) = 0;

  virtual void enterArguments(azslParser::ArgumentsContext *ctx) = 0;
  virtual void exitArguments(azslParser::ArgumentsContext *ctx) = 0;

  virtual void enterVariableDeclaration(azslParser::VariableDeclarationContext *ctx) = 0;
  virtual void exitVariableDeclaration(azslParser::VariableDeclarationContext *ctx) = 0;

  virtual void enterVariableDeclarators(azslParser::VariableDeclaratorsContext *ctx) = 0;
  virtual void exitVariableDeclarators(azslParser::VariableDeclaratorsContext *ctx) = 0;

  virtual void enterUnnamedVariableDeclarator(azslParser::UnnamedVariableDeclaratorContext *ctx) = 0;
  virtual void exitUnnamedVariableDeclarator(azslParser::UnnamedVariableDeclaratorContext *ctx) = 0;

  virtual void enterNamedVariableDeclarator(azslParser::NamedVariableDeclaratorContext *ctx) = 0;
  virtual void exitNamedVariableDeclarator(azslParser::NamedVariableDeclaratorContext *ctx) = 0;

  virtual void enterVariableInitializer(azslParser::VariableInitializerContext *ctx) = 0;
  virtual void exitVariableInitializer(azslParser::VariableInitializerContext *ctx) = 0;

  virtual void enterStandardVariableInitializer(azslParser::StandardVariableInitializerContext *ctx) = 0;
  virtual void exitStandardVariableInitializer(azslParser::StandardVariableInitializerContext *ctx) = 0;

  virtual void enterArrayElementInitializers(azslParser::ArrayElementInitializersContext *ctx) = 0;
  virtual void exitArrayElementInitializers(azslParser::ArrayElementInitializersContext *ctx) = 0;

  virtual void enterArrayRankSpecifier(azslParser::ArrayRankSpecifierContext *ctx) = 0;
  virtual void exitArrayRankSpecifier(azslParser::ArrayRankSpecifierContext *ctx) = 0;

  virtual void enterPackOffsetNode(azslParser::PackOffsetNodeContext *ctx) = 0;
  virtual void exitPackOffsetNode(azslParser::PackOffsetNodeContext *ctx) = 0;

  virtual void enterStorageFlags(azslParser::StorageFlagsContext *ctx) = 0;
  virtual void exitStorageFlags(azslParser::StorageFlagsContext *ctx) = 0;

  virtual void enterStorageFlag(azslParser::StorageFlagContext *ctx) = 0;
  virtual void exitStorageFlag(azslParser::StorageFlagContext *ctx) = 0;

  virtual void enterType(azslParser::TypeContext *ctx) = 0;
  virtual void exitType(azslParser::TypeContext *ctx) = 0;

  virtual void enterPredefinedType(azslParser::PredefinedTypeContext *ctx) = 0;
  virtual void exitPredefinedType(azslParser::PredefinedTypeContext *ctx) = 0;

  virtual void enterSubobjectType(azslParser::SubobjectTypeContext *ctx) = 0;
  virtual void exitSubobjectType(azslParser::SubobjectTypeContext *ctx) = 0;

  virtual void enterOtherViewResourceType(azslParser::OtherViewResourceTypeContext *ctx) = 0;
  virtual void exitOtherViewResourceType(azslParser::OtherViewResourceTypeContext *ctx) = 0;

  virtual void enterRtxBuiltInTypes(azslParser::RtxBuiltInTypesContext *ctx) = 0;
  virtual void exitRtxBuiltInTypes(azslParser::RtxBuiltInTypesContext *ctx) = 0;

  virtual void enterBufferPredefinedType(azslParser::BufferPredefinedTypeContext *ctx) = 0;
  virtual void exitBufferPredefinedType(azslParser::BufferPredefinedTypeContext *ctx) = 0;

  virtual void enterBufferType(azslParser::BufferTypeContext *ctx) = 0;
  virtual void exitBufferType(azslParser::BufferTypeContext *ctx) = 0;

  virtual void enterByteAddressBufferTypes(azslParser::ByteAddressBufferTypesContext *ctx) = 0;
  virtual void exitByteAddressBufferTypes(azslParser::ByteAddressBufferTypesContext *ctx) = 0;

  virtual void enterPatchPredefinedType(azslParser::PatchPredefinedTypeContext *ctx) = 0;
  virtual void exitPatchPredefinedType(azslParser::PatchPredefinedTypeContext *ctx) = 0;

  virtual void enterPatchType(azslParser::PatchTypeContext *ctx) = 0;
  virtual void exitPatchType(azslParser::PatchTypeContext *ctx) = 0;

  virtual void enterSamplerStatePredefinedType(azslParser::SamplerStatePredefinedTypeContext *ctx) = 0;
  virtual void exitSamplerStatePredefinedType(azslParser::SamplerStatePredefinedTypeContext *ctx) = 0;

  virtual void enterScalarType(azslParser::ScalarTypeContext *ctx) = 0;
  virtual void exitScalarType(azslParser::ScalarTypeContext *ctx) = 0;

  virtual void enterStreamOutputPredefinedType(azslParser::StreamOutputPredefinedTypeContext *ctx) = 0;
  virtual void exitStreamOutputPredefinedType(azslParser::StreamOutputPredefinedTypeContext *ctx) = 0;

  virtual void enterStreamOutputObjectType(azslParser::StreamOutputObjectTypeContext *ctx) = 0;
  virtual void exitStreamOutputObjectType(azslParser::StreamOutputObjectTypeContext *ctx) = 0;

  virtual void enterStructuredBufferPredefinedType(azslParser::StructuredBufferPredefinedTypeContext *ctx) = 0;
  virtual void exitStructuredBufferPredefinedType(azslParser::StructuredBufferPredefinedTypeContext *ctx) = 0;

  virtual void enterStructuredBufferName(azslParser::StructuredBufferNameContext *ctx) = 0;
  virtual void exitStructuredBufferName(azslParser::StructuredBufferNameContext *ctx) = 0;

  virtual void enterTextureType(azslParser::TextureTypeContext *ctx) = 0;
  virtual void exitTextureType(azslParser::TextureTypeContext *ctx) = 0;

  virtual void enterTexturePredefinedType(azslParser::TexturePredefinedTypeContext *ctx) = 0;
  virtual void exitTexturePredefinedType(azslParser::TexturePredefinedTypeContext *ctx) = 0;

  virtual void enterGenericTexturePredefinedType(azslParser::GenericTexturePredefinedTypeContext *ctx) = 0;
  virtual void exitGenericTexturePredefinedType(azslParser::GenericTexturePredefinedTypeContext *ctx) = 0;

  virtual void enterTextureTypeMS(azslParser::TextureTypeMSContext *ctx) = 0;
  virtual void exitTextureTypeMS(azslParser::TextureTypeMSContext *ctx) = 0;

  virtual void enterMsTexturePredefinedType(azslParser::MsTexturePredefinedTypeContext *ctx) = 0;
  virtual void exitMsTexturePredefinedType(azslParser::MsTexturePredefinedTypeContext *ctx) = 0;

  virtual void enterSubpassInputType(azslParser::SubpassInputTypeContext *ctx) = 0;
  virtual void exitSubpassInputType(azslParser::SubpassInputTypeContext *ctx) = 0;

  virtual void enterSubpassInputPredefinedType(azslParser::SubpassInputPredefinedTypeContext *ctx) = 0;
  virtual void exitSubpassInputPredefinedType(azslParser::SubpassInputPredefinedTypeContext *ctx) = 0;

  virtual void enterGenericSubpassInputPredefinedType(azslParser::GenericSubpassInputPredefinedTypeContext *ctx) = 0;
  virtual void exitGenericSubpassInputPredefinedType(azslParser::GenericSubpassInputPredefinedTypeContext *ctx) = 0;

  virtual void enterVectorType(azslParser::VectorTypeContext *ctx) = 0;
  virtual void exitVectorType(azslParser::VectorTypeContext *ctx) = 0;

  virtual void enterGenericVectorType(azslParser::GenericVectorTypeContext *ctx) = 0;
  virtual void exitGenericVectorType(azslParser::GenericVectorTypeContext *ctx) = 0;

  virtual void enterScalarOrVectorType(azslParser::ScalarOrVectorTypeContext *ctx) = 0;
  virtual void exitScalarOrVectorType(azslParser::ScalarOrVectorTypeContext *ctx) = 0;

  virtual void enterScalarOrVectorOrMatrixType(azslParser::ScalarOrVectorOrMatrixTypeContext *ctx) = 0;
  virtual void exitScalarOrVectorOrMatrixType(azslParser::ScalarOrVectorOrMatrixTypeContext *ctx) = 0;

  virtual void enterMatrixType(azslParser::MatrixTypeContext *ctx) = 0;
  virtual void exitMatrixType(azslParser::MatrixTypeContext *ctx) = 0;

  virtual void enterGenericMatrixPredefinedType(azslParser::GenericMatrixPredefinedTypeContext *ctx) = 0;
  virtual void exitGenericMatrixPredefinedType(azslParser::GenericMatrixPredefinedTypeContext *ctx) = 0;

  virtual void enterRegisterAllocation(azslParser::RegisterAllocationContext *ctx) = 0;
  virtual void exitRegisterAllocation(azslParser::RegisterAllocationContext *ctx) = 0;

  virtual void enterSamplerStateProperty(azslParser::SamplerStatePropertyContext *ctx) = 0;
  virtual void exitSamplerStateProperty(azslParser::SamplerStatePropertyContext *ctx) = 0;

  virtual void enterLiteral(azslParser::LiteralContext *ctx) = 0;
  virtual void exitLiteral(azslParser::LiteralContext *ctx) = 0;

  virtual void enterLeadingTypeFunctionSignature(azslParser::LeadingTypeFunctionSignatureContext *ctx) = 0;
  virtual void exitLeadingTypeFunctionSignature(azslParser::LeadingTypeFunctionSignatureContext *ctx) = 0;

  virtual void enterHlslFunctionDefinition(azslParser::HlslFunctionDefinitionContext *ctx) = 0;
  virtual void exitHlslFunctionDefinition(azslParser::HlslFunctionDefinitionContext *ctx) = 0;

  virtual void enterHlslFunctionDeclaration(azslParser::HlslFunctionDeclarationContext *ctx) = 0;
  virtual void exitHlslFunctionDeclaration(azslParser::HlslFunctionDeclarationContext *ctx) = 0;

  virtual void enterUserDefinedType(azslParser::UserDefinedTypeContext *ctx) = 0;
  virtual void exitUserDefinedType(azslParser::UserDefinedTypeContext *ctx) = 0;

  virtual void enterAssociatedTypeDeclaration(azslParser::AssociatedTypeDeclarationContext *ctx) = 0;
  virtual void exitAssociatedTypeDeclaration(azslParser::AssociatedTypeDeclarationContext *ctx) = 0;

  virtual void enterTypedefStatement(azslParser::TypedefStatementContext *ctx) = 0;
  virtual void exitTypedefStatement(azslParser::TypedefStatementContext *ctx) = 0;

  virtual void enterTypealiasStatement(azslParser::TypealiasStatementContext *ctx) = 0;
  virtual void exitTypealiasStatement(azslParser::TypealiasStatementContext *ctx) = 0;

  virtual void enterTypeAliasingDefinitionStatement(azslParser::TypeAliasingDefinitionStatementContext *ctx) = 0;
  virtual void exitTypeAliasingDefinitionStatement(azslParser::TypeAliasingDefinitionStatementContext *ctx) = 0;

  virtual void enterTypeofExpression(azslParser::TypeofExpressionContext *ctx) = 0;
  virtual void exitTypeofExpression(azslParser::TypeofExpressionContext *ctx) = 0;

  virtual void enterGenericParameterList(azslParser::GenericParameterListContext *ctx) = 0;
  virtual void exitGenericParameterList(azslParser::GenericParameterListContext *ctx) = 0;

  virtual void enterGenericTypeDefinition(azslParser::GenericTypeDefinitionContext *ctx) = 0;
  virtual void exitGenericTypeDefinition(azslParser::GenericTypeDefinitionContext *ctx) = 0;

  virtual void enterGenericConstraint(azslParser::GenericConstraintContext *ctx) = 0;
  virtual void exitGenericConstraint(azslParser::GenericConstraintContext *ctx) = 0;

  virtual void enterLanguageDefinedConstraint(azslParser::LanguageDefinedConstraintContext *ctx) = 0;
  virtual void exitLanguageDefinedConstraint(azslParser::LanguageDefinedConstraintContext *ctx) = 0;

  virtual void enterFunctionDeclaration(azslParser::FunctionDeclarationContext *ctx) = 0;
  virtual void exitFunctionDeclaration(azslParser::FunctionDeclarationContext *ctx) = 0;

  virtual void enterAttributedFunctionDeclaration(azslParser::AttributedFunctionDeclarationContext *ctx) = 0;
  virtual void exitAttributedFunctionDeclaration(azslParser::AttributedFunctionDeclarationContext *ctx) = 0;

  virtual void enterFunctionDefinition(azslParser::FunctionDefinitionContext *ctx) = 0;
  virtual void exitFunctionDefinition(azslParser::FunctionDefinitionContext *ctx) = 0;

  virtual void enterAttributedFunctionDefinition(azslParser::AttributedFunctionDefinitionContext *ctx) = 0;
  virtual void exitAttributedFunctionDefinition(azslParser::AttributedFunctionDefinitionContext *ctx) = 0;

  virtual void enterCompilerExtensionStatement(azslParser::CompilerExtensionStatementContext *ctx) = 0;
  virtual void exitCompilerExtensionStatement(azslParser::CompilerExtensionStatementContext *ctx) = 0;

  virtual void enterSrgDefinition(azslParser::SrgDefinitionContext *ctx) = 0;
  virtual void exitSrgDefinition(azslParser::SrgDefinitionContext *ctx) = 0;

  virtual void enterAttributedSrgDefinition(azslParser::AttributedSrgDefinitionContext *ctx) = 0;
  virtual void exitAttributedSrgDefinition(azslParser::AttributedSrgDefinitionContext *ctx) = 0;

  virtual void enterSrgMemberDeclaration(azslParser::SrgMemberDeclarationContext *ctx) = 0;
  virtual void exitSrgMemberDeclaration(azslParser::SrgMemberDeclarationContext *ctx) = 0;

  virtual void enterSrgSemantic(azslParser::SrgSemanticContext *ctx) = 0;
  virtual void exitSrgSemantic(azslParser::SrgSemanticContext *ctx) = 0;

  virtual void enterAttributedSrgSemantic(azslParser::AttributedSrgSemanticContext *ctx) = 0;
  virtual void exitAttributedSrgSemantic(azslParser::AttributedSrgSemanticContext *ctx) = 0;

  virtual void enterSrgSemanticBodyDeclaration(azslParser::SrgSemanticBodyDeclarationContext *ctx) = 0;
  virtual void exitSrgSemanticBodyDeclaration(azslParser::SrgSemanticBodyDeclarationContext *ctx) = 0;

  virtual void enterSrgSemanticMemberDeclaration(azslParser::SrgSemanticMemberDeclarationContext *ctx) = 0;
  virtual void exitSrgSemanticMemberDeclaration(azslParser::SrgSemanticMemberDeclarationContext *ctx) = 0;

  virtual void enterSamplerBodyDeclaration(azslParser::SamplerBodyDeclarationContext *ctx) = 0;
  virtual void exitSamplerBodyDeclaration(azslParser::SamplerBodyDeclarationContext *ctx) = 0;

  virtual void enterSamplerMemberDeclaration(azslParser::SamplerMemberDeclarationContext *ctx) = 0;
  virtual void exitSamplerMemberDeclaration(azslParser::SamplerMemberDeclarationContext *ctx) = 0;

  virtual void enterMaxAnisotropyOption(azslParser::MaxAnisotropyOptionContext *ctx) = 0;
  virtual void exitMaxAnisotropyOption(azslParser::MaxAnisotropyOptionContext *ctx) = 0;

  virtual void enterMinFilterOption(azslParser::MinFilterOptionContext *ctx) = 0;
  virtual void exitMinFilterOption(azslParser::MinFilterOptionContext *ctx) = 0;

  virtual void enterMagFilterOption(azslParser::MagFilterOptionContext *ctx) = 0;
  virtual void exitMagFilterOption(azslParser::MagFilterOptionContext *ctx) = 0;

  virtual void enterMipFilterOption(azslParser::MipFilterOptionContext *ctx) = 0;
  virtual void exitMipFilterOption(azslParser::MipFilterOptionContext *ctx) = 0;

  virtual void enterReductionTypeOption(azslParser::ReductionTypeOptionContext *ctx) = 0;
  virtual void exitReductionTypeOption(azslParser::ReductionTypeOptionContext *ctx) = 0;

  virtual void enterComparisonFunctionOption(azslParser::ComparisonFunctionOptionContext *ctx) = 0;
  virtual void exitComparisonFunctionOption(azslParser::ComparisonFunctionOptionContext *ctx) = 0;

  virtual void enterAddressUOption(azslParser::AddressUOptionContext *ctx) = 0;
  virtual void exitAddressUOption(azslParser::AddressUOptionContext *ctx) = 0;

  virtual void enterAddressVOption(azslParser::AddressVOptionContext *ctx) = 0;
  virtual void exitAddressVOption(azslParser::AddressVOptionContext *ctx) = 0;

  virtual void enterAddressWOption(azslParser::AddressWOptionContext *ctx) = 0;
  virtual void exitAddressWOption(azslParser::AddressWOptionContext *ctx) = 0;

  virtual void enterMinLodOption(azslParser::MinLodOptionContext *ctx) = 0;
  virtual void exitMinLodOption(azslParser::MinLodOptionContext *ctx) = 0;

  virtual void enterMaxLodOption(azslParser::MaxLodOptionContext *ctx) = 0;
  virtual void exitMaxLodOption(azslParser::MaxLodOptionContext *ctx) = 0;

  virtual void enterMipLodBiasOption(azslParser::MipLodBiasOptionContext *ctx) = 0;
  virtual void exitMipLodBiasOption(azslParser::MipLodBiasOptionContext *ctx) = 0;

  virtual void enterBorderColorOption(azslParser::BorderColorOptionContext *ctx) = 0;
  virtual void exitBorderColorOption(azslParser::BorderColorOptionContext *ctx) = 0;

  virtual void enterFilterModeEnum(azslParser::FilterModeEnumContext *ctx) = 0;
  virtual void exitFilterModeEnum(azslParser::FilterModeEnumContext *ctx) = 0;

  virtual void enterReductionTypeEnum(azslParser::ReductionTypeEnumContext *ctx) = 0;
  virtual void exitReductionTypeEnum(azslParser::ReductionTypeEnumContext *ctx) = 0;

  virtual void enterAddressModeEnum(azslParser::AddressModeEnumContext *ctx) = 0;
  virtual void exitAddressModeEnum(azslParser::AddressModeEnumContext *ctx) = 0;

  virtual void enterComparisonFunctionEnum(azslParser::ComparisonFunctionEnumContext *ctx) = 0;
  virtual void exitComparisonFunctionEnum(azslParser::ComparisonFunctionEnumContext *ctx) = 0;

  virtual void enterBorderColorEnum(azslParser::BorderColorEnumContext *ctx) = 0;
  virtual void exitBorderColorEnum(azslParser::BorderColorEnumContext *ctx) = 0;


};

