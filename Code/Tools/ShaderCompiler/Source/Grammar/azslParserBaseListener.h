// Copyright (c) Contributors to the Open 3D Engine Project.
// For complete copyright and license terms please see the LICENSE at the root of this distribution.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT
//

// Generated from azslParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "azslParserListener.h"


/**
 * This class provides an empty implementation of azslParserListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  azslParserBaseListener : public azslParserListener {
public:

  virtual void enterCompilationUnit(azslParser::CompilationUnitContext * /*ctx*/) override { }
  virtual void exitCompilationUnit(azslParser::CompilationUnitContext * /*ctx*/) override { }

  virtual void enterTopLevelDeclaration(azslParser::TopLevelDeclarationContext * /*ctx*/) override { }
  virtual void exitTopLevelDeclaration(azslParser::TopLevelDeclarationContext * /*ctx*/) override { }

  virtual void enterIdExpression(azslParser::IdExpressionContext * /*ctx*/) override { }
  virtual void exitIdExpression(azslParser::IdExpressionContext * /*ctx*/) override { }

  virtual void enterUnqualifiedId(azslParser::UnqualifiedIdContext * /*ctx*/) override { }
  virtual void exitUnqualifiedId(azslParser::UnqualifiedIdContext * /*ctx*/) override { }

  virtual void enterQualifiedId(azslParser::QualifiedIdContext * /*ctx*/) override { }
  virtual void exitQualifiedId(azslParser::QualifiedIdContext * /*ctx*/) override { }

  virtual void enterNestedNameSpecifier(azslParser::NestedNameSpecifierContext * /*ctx*/) override { }
  virtual void exitNestedNameSpecifier(azslParser::NestedNameSpecifierContext * /*ctx*/) override { }

  virtual void enterClassDefinitionStatement(azslParser::ClassDefinitionStatementContext * /*ctx*/) override { }
  virtual void exitClassDefinitionStatement(azslParser::ClassDefinitionStatementContext * /*ctx*/) override { }

  virtual void enterClassDefinition(azslParser::ClassDefinitionContext * /*ctx*/) override { }
  virtual void exitClassDefinition(azslParser::ClassDefinitionContext * /*ctx*/) override { }

  virtual void enterBaseList(azslParser::BaseListContext * /*ctx*/) override { }
  virtual void exitBaseList(azslParser::BaseListContext * /*ctx*/) override { }

  virtual void enterClassMemberDeclaration(azslParser::ClassMemberDeclarationContext * /*ctx*/) override { }
  virtual void exitClassMemberDeclaration(azslParser::ClassMemberDeclarationContext * /*ctx*/) override { }

  virtual void enterStructDefinitionStatement(azslParser::StructDefinitionStatementContext * /*ctx*/) override { }
  virtual void exitStructDefinitionStatement(azslParser::StructDefinitionStatementContext * /*ctx*/) override { }

  virtual void enterStructDefinition(azslParser::StructDefinitionContext * /*ctx*/) override { }
  virtual void exitStructDefinition(azslParser::StructDefinitionContext * /*ctx*/) override { }

  virtual void enterStructMemberDeclaration(azslParser::StructMemberDeclarationContext * /*ctx*/) override { }
  virtual void exitStructMemberDeclaration(azslParser::StructMemberDeclarationContext * /*ctx*/) override { }

  virtual void enterAnyStructuredTypeDefinitionStatement(azslParser::AnyStructuredTypeDefinitionStatementContext * /*ctx*/) override { }
  virtual void exitAnyStructuredTypeDefinitionStatement(azslParser::AnyStructuredTypeDefinitionStatementContext * /*ctx*/) override { }

  virtual void enterEnumDefinitionStatement(azslParser::EnumDefinitionStatementContext * /*ctx*/) override { }
  virtual void exitEnumDefinitionStatement(azslParser::EnumDefinitionStatementContext * /*ctx*/) override { }

  virtual void enterEnumDefinition(azslParser::EnumDefinitionContext * /*ctx*/) override { }
  virtual void exitEnumDefinition(azslParser::EnumDefinitionContext * /*ctx*/) override { }

  virtual void enterUnscopedEnum(azslParser::UnscopedEnumContext * /*ctx*/) override { }
  virtual void exitUnscopedEnum(azslParser::UnscopedEnumContext * /*ctx*/) override { }

  virtual void enterScopedEnum(azslParser::ScopedEnumContext * /*ctx*/) override { }
  virtual void exitScopedEnum(azslParser::ScopedEnumContext * /*ctx*/) override { }

  virtual void enterEnumeratorListDefinition(azslParser::EnumeratorListDefinitionContext * /*ctx*/) override { }
  virtual void exitEnumeratorListDefinition(azslParser::EnumeratorListDefinitionContext * /*ctx*/) override { }

  virtual void enterEnumeratorDeclarator(azslParser::EnumeratorDeclaratorContext * /*ctx*/) override { }
  virtual void exitEnumeratorDeclarator(azslParser::EnumeratorDeclaratorContext * /*ctx*/) override { }

  virtual void enterAnyStructuredTypeDefinition(azslParser::AnyStructuredTypeDefinitionContext * /*ctx*/) override { }
  virtual void exitAnyStructuredTypeDefinition(azslParser::AnyStructuredTypeDefinitionContext * /*ctx*/) override { }

  virtual void enterInterfaceDefinitionStatement(azslParser::InterfaceDefinitionStatementContext * /*ctx*/) override { }
  virtual void exitInterfaceDefinitionStatement(azslParser::InterfaceDefinitionStatementContext * /*ctx*/) override { }

  virtual void enterInterfaceDefinition(azslParser::InterfaceDefinitionContext * /*ctx*/) override { }
  virtual void exitInterfaceDefinition(azslParser::InterfaceDefinitionContext * /*ctx*/) override { }

  virtual void enterInterfaceMemberDeclaration(azslParser::InterfaceMemberDeclarationContext * /*ctx*/) override { }
  virtual void exitInterfaceMemberDeclaration(azslParser::InterfaceMemberDeclarationContext * /*ctx*/) override { }

  virtual void enterConstantBufferTemplated(azslParser::ConstantBufferTemplatedContext * /*ctx*/) override { }
  virtual void exitConstantBufferTemplated(azslParser::ConstantBufferTemplatedContext * /*ctx*/) override { }

  virtual void enterVariableDeclarationStatement(azslParser::VariableDeclarationStatementContext * /*ctx*/) override { }
  virtual void exitVariableDeclarationStatement(azslParser::VariableDeclarationStatementContext * /*ctx*/) override { }

  virtual void enterFunctionParams(azslParser::FunctionParamsContext * /*ctx*/) override { }
  virtual void exitFunctionParams(azslParser::FunctionParamsContext * /*ctx*/) override { }

  virtual void enterFunctionParam(azslParser::FunctionParamContext * /*ctx*/) override { }
  virtual void exitFunctionParam(azslParser::FunctionParamContext * /*ctx*/) override { }

  virtual void enterHlslSemantic(azslParser::HlslSemanticContext * /*ctx*/) override { }
  virtual void exitHlslSemantic(azslParser::HlslSemanticContext * /*ctx*/) override { }

  virtual void enterHlslSemanticName(azslParser::HlslSemanticNameContext * /*ctx*/) override { }
  virtual void exitHlslSemanticName(azslParser::HlslSemanticNameContext * /*ctx*/) override { }

  virtual void enterAttributeArguments(azslParser::AttributeArgumentsContext * /*ctx*/) override { }
  virtual void exitAttributeArguments(azslParser::AttributeArgumentsContext * /*ctx*/) override { }

  virtual void enterAttributeArgumentList(azslParser::AttributeArgumentListContext * /*ctx*/) override { }
  virtual void exitAttributeArgumentList(azslParser::AttributeArgumentListContext * /*ctx*/) override { }

  virtual void enterGlobalAttribute(azslParser::GlobalAttributeContext * /*ctx*/) override { }
  virtual void exitGlobalAttribute(azslParser::GlobalAttributeContext * /*ctx*/) override { }

  virtual void enterAttachedAttribute(azslParser::AttachedAttributeContext * /*ctx*/) override { }
  virtual void exitAttachedAttribute(azslParser::AttachedAttributeContext * /*ctx*/) override { }

  virtual void enterAttributeSpecifier(azslParser::AttributeSpecifierContext * /*ctx*/) override { }
  virtual void exitAttributeSpecifier(azslParser::AttributeSpecifierContext * /*ctx*/) override { }

  virtual void enterAttributeSpecifierSequence(azslParser::AttributeSpecifierSequenceContext * /*ctx*/) override { }
  virtual void exitAttributeSpecifierSequence(azslParser::AttributeSpecifierSequenceContext * /*ctx*/) override { }

  virtual void enterAttributeSpecifierAny(azslParser::AttributeSpecifierAnyContext * /*ctx*/) override { }
  virtual void exitAttributeSpecifierAny(azslParser::AttributeSpecifierAnyContext * /*ctx*/) override { }

  virtual void enterBlock(azslParser::BlockContext * /*ctx*/) override { }
  virtual void exitBlock(azslParser::BlockContext * /*ctx*/) override { }

  virtual void enterStatement(azslParser::StatementContext * /*ctx*/) override { }
  virtual void exitStatement(azslParser::StatementContext * /*ctx*/) override { }

  virtual void enterForInitializer(azslParser::ForInitializerContext * /*ctx*/) override { }
  virtual void exitForInitializer(azslParser::ForInitializerContext * /*ctx*/) override { }

  virtual void enterCaseSwitchLabel(azslParser::CaseSwitchLabelContext * /*ctx*/) override { }
  virtual void exitCaseSwitchLabel(azslParser::CaseSwitchLabelContext * /*ctx*/) override { }

  virtual void enterDefaultSwitchLabel(azslParser::DefaultSwitchLabelContext * /*ctx*/) override { }
  virtual void exitDefaultSwitchLabel(azslParser::DefaultSwitchLabelContext * /*ctx*/) override { }

  virtual void enterSwitchSection(azslParser::SwitchSectionContext * /*ctx*/) override { }
  virtual void exitSwitchSection(azslParser::SwitchSectionContext * /*ctx*/) override { }

  virtual void enterSwitchBlock(azslParser::SwitchBlockContext * /*ctx*/) override { }
  virtual void exitSwitchBlock(azslParser::SwitchBlockContext * /*ctx*/) override { }

  virtual void enterEmptyStatement(azslParser::EmptyStatementContext * /*ctx*/) override { }
  virtual void exitEmptyStatement(azslParser::EmptyStatementContext * /*ctx*/) override { }

  virtual void enterBlockStatement(azslParser::BlockStatementContext * /*ctx*/) override { }
  virtual void exitBlockStatement(azslParser::BlockStatementContext * /*ctx*/) override { }

  virtual void enterExpressionStatement(azslParser::ExpressionStatementContext * /*ctx*/) override { }
  virtual void exitExpressionStatement(azslParser::ExpressionStatementContext * /*ctx*/) override { }

  virtual void enterIfStatement(azslParser::IfStatementContext * /*ctx*/) override { }
  virtual void exitIfStatement(azslParser::IfStatementContext * /*ctx*/) override { }

  virtual void enterSwitchStatement(azslParser::SwitchStatementContext * /*ctx*/) override { }
  virtual void exitSwitchStatement(azslParser::SwitchStatementContext * /*ctx*/) override { }

  virtual void enterWhileStatement(azslParser::WhileStatementContext * /*ctx*/) override { }
  virtual void exitWhileStatement(azslParser::WhileStatementContext * /*ctx*/) override { }

  virtual void enterDoStatement(azslParser::DoStatementContext * /*ctx*/) override { }
  virtual void exitDoStatement(azslParser::DoStatementContext * /*ctx*/) override { }

  virtual void enterForStatement(azslParser::ForStatementContext * /*ctx*/) override { }
  virtual void exitForStatement(azslParser::ForStatementContext * /*ctx*/) override { }

  virtual void enterBreakStatement(azslParser::BreakStatementContext * /*ctx*/) override { }
  virtual void exitBreakStatement(azslParser::BreakStatementContext * /*ctx*/) override { }

  virtual void enterContinueStatement(azslParser::ContinueStatementContext * /*ctx*/) override { }
  virtual void exitContinueStatement(azslParser::ContinueStatementContext * /*ctx*/) override { }

  virtual void enterDiscardStatement(azslParser::DiscardStatementContext * /*ctx*/) override { }
  virtual void exitDiscardStatement(azslParser::DiscardStatementContext * /*ctx*/) override { }

  virtual void enterReturnStatement(azslParser::ReturnStatementContext * /*ctx*/) override { }
  virtual void exitReturnStatement(azslParser::ReturnStatementContext * /*ctx*/) override { }

  virtual void enterExtenstionStatement(azslParser::ExtenstionStatementContext * /*ctx*/) override { }
  virtual void exitExtenstionStatement(azslParser::ExtenstionStatementContext * /*ctx*/) override { }

  virtual void enterTypeAliasingDefinitionStatementLabel(azslParser::TypeAliasingDefinitionStatementLabelContext * /*ctx*/) override { }
  virtual void exitTypeAliasingDefinitionStatementLabel(azslParser::TypeAliasingDefinitionStatementLabelContext * /*ctx*/) override { }

  virtual void enterElseClause(azslParser::ElseClauseContext * /*ctx*/) override { }
  virtual void exitElseClause(azslParser::ElseClauseContext * /*ctx*/) override { }

  virtual void enterParenthesizedExpression(azslParser::ParenthesizedExpressionContext * /*ctx*/) override { }
  virtual void exitParenthesizedExpression(azslParser::ParenthesizedExpressionContext * /*ctx*/) override { }

  virtual void enterMemberAccessExpression(azslParser::MemberAccessExpressionContext * /*ctx*/) override { }
  virtual void exitMemberAccessExpression(azslParser::MemberAccessExpressionContext * /*ctx*/) override { }

  virtual void enterPrefixUnaryExpression(azslParser::PrefixUnaryExpressionContext * /*ctx*/) override { }
  virtual void exitPrefixUnaryExpression(azslParser::PrefixUnaryExpressionContext * /*ctx*/) override { }

  virtual void enterLiteralExpression(azslParser::LiteralExpressionContext * /*ctx*/) override { }
  virtual void exitLiteralExpression(azslParser::LiteralExpressionContext * /*ctx*/) override { }

  virtual void enterConditionalExpression(azslParser::ConditionalExpressionContext * /*ctx*/) override { }
  virtual void exitConditionalExpression(azslParser::ConditionalExpressionContext * /*ctx*/) override { }

  virtual void enterPostfixUnaryExpression(azslParser::PostfixUnaryExpressionContext * /*ctx*/) override { }
  virtual void exitPostfixUnaryExpression(azslParser::PostfixUnaryExpressionContext * /*ctx*/) override { }

  virtual void enterNumericConstructorExpression(azslParser::NumericConstructorExpressionContext * /*ctx*/) override { }
  virtual void exitNumericConstructorExpression(azslParser::NumericConstructorExpressionContext * /*ctx*/) override { }

  virtual void enterFunctionCallExpression(azslParser::FunctionCallExpressionContext * /*ctx*/) override { }
  virtual void exitFunctionCallExpression(azslParser::FunctionCallExpressionContext * /*ctx*/) override { }

  virtual void enterIdentifierExpression(azslParser::IdentifierExpressionContext * /*ctx*/) override { }
  virtual void exitIdentifierExpression(azslParser::IdentifierExpressionContext * /*ctx*/) override { }

  virtual void enterBinaryExpression(azslParser::BinaryExpressionContext * /*ctx*/) override { }
  virtual void exitBinaryExpression(azslParser::BinaryExpressionContext * /*ctx*/) override { }

  virtual void enterAssignmentExpression(azslParser::AssignmentExpressionContext * /*ctx*/) override { }
  virtual void exitAssignmentExpression(azslParser::AssignmentExpressionContext * /*ctx*/) override { }

  virtual void enterCastExpression(azslParser::CastExpressionContext * /*ctx*/) override { }
  virtual void exitCastExpression(azslParser::CastExpressionContext * /*ctx*/) override { }

  virtual void enterArrayAccessExpression(azslParser::ArrayAccessExpressionContext * /*ctx*/) override { }
  virtual void exitArrayAccessExpression(azslParser::ArrayAccessExpressionContext * /*ctx*/) override { }

  virtual void enterOtherExpression(azslParser::OtherExpressionContext * /*ctx*/) override { }
  virtual void exitOtherExpression(azslParser::OtherExpressionContext * /*ctx*/) override { }

  virtual void enterCommaExpression(azslParser::CommaExpressionContext * /*ctx*/) override { }
  virtual void exitCommaExpression(azslParser::CommaExpressionContext * /*ctx*/) override { }

  virtual void enterPostfixUnaryOperator(azslParser::PostfixUnaryOperatorContext * /*ctx*/) override { }
  virtual void exitPostfixUnaryOperator(azslParser::PostfixUnaryOperatorContext * /*ctx*/) override { }

  virtual void enterPrefixUnaryOperator(azslParser::PrefixUnaryOperatorContext * /*ctx*/) override { }
  virtual void exitPrefixUnaryOperator(azslParser::PrefixUnaryOperatorContext * /*ctx*/) override { }

  virtual void enterBinaryOperator(azslParser::BinaryOperatorContext * /*ctx*/) override { }
  virtual void exitBinaryOperator(azslParser::BinaryOperatorContext * /*ctx*/) override { }

  virtual void enterAssignmentOperator(azslParser::AssignmentOperatorContext * /*ctx*/) override { }
  virtual void exitAssignmentOperator(azslParser::AssignmentOperatorContext * /*ctx*/) override { }

  virtual void enterArgumentList(azslParser::ArgumentListContext * /*ctx*/) override { }
  virtual void exitArgumentList(azslParser::ArgumentListContext * /*ctx*/) override { }

  virtual void enterArguments(azslParser::ArgumentsContext * /*ctx*/) override { }
  virtual void exitArguments(azslParser::ArgumentsContext * /*ctx*/) override { }

  virtual void enterVariableDeclaration(azslParser::VariableDeclarationContext * /*ctx*/) override { }
  virtual void exitVariableDeclaration(azslParser::VariableDeclarationContext * /*ctx*/) override { }

  virtual void enterVariableDeclarators(azslParser::VariableDeclaratorsContext * /*ctx*/) override { }
  virtual void exitVariableDeclarators(azslParser::VariableDeclaratorsContext * /*ctx*/) override { }

  virtual void enterUnnamedVariableDeclarator(azslParser::UnnamedVariableDeclaratorContext * /*ctx*/) override { }
  virtual void exitUnnamedVariableDeclarator(azslParser::UnnamedVariableDeclaratorContext * /*ctx*/) override { }

  virtual void enterNamedVariableDeclarator(azslParser::NamedVariableDeclaratorContext * /*ctx*/) override { }
  virtual void exitNamedVariableDeclarator(azslParser::NamedVariableDeclaratorContext * /*ctx*/) override { }

  virtual void enterVariableInitializer(azslParser::VariableInitializerContext * /*ctx*/) override { }
  virtual void exitVariableInitializer(azslParser::VariableInitializerContext * /*ctx*/) override { }

  virtual void enterStandardVariableInitializer(azslParser::StandardVariableInitializerContext * /*ctx*/) override { }
  virtual void exitStandardVariableInitializer(azslParser::StandardVariableInitializerContext * /*ctx*/) override { }

  virtual void enterArrayElementInitializers(azslParser::ArrayElementInitializersContext * /*ctx*/) override { }
  virtual void exitArrayElementInitializers(azslParser::ArrayElementInitializersContext * /*ctx*/) override { }

  virtual void enterArrayRankSpecifier(azslParser::ArrayRankSpecifierContext * /*ctx*/) override { }
  virtual void exitArrayRankSpecifier(azslParser::ArrayRankSpecifierContext * /*ctx*/) override { }

  virtual void enterPackOffsetNode(azslParser::PackOffsetNodeContext * /*ctx*/) override { }
  virtual void exitPackOffsetNode(azslParser::PackOffsetNodeContext * /*ctx*/) override { }

  virtual void enterStorageFlags(azslParser::StorageFlagsContext * /*ctx*/) override { }
  virtual void exitStorageFlags(azslParser::StorageFlagsContext * /*ctx*/) override { }

  virtual void enterStorageFlag(azslParser::StorageFlagContext * /*ctx*/) override { }
  virtual void exitStorageFlag(azslParser::StorageFlagContext * /*ctx*/) override { }

  virtual void enterType(azslParser::TypeContext * /*ctx*/) override { }
  virtual void exitType(azslParser::TypeContext * /*ctx*/) override { }

  virtual void enterPredefinedType(azslParser::PredefinedTypeContext * /*ctx*/) override { }
  virtual void exitPredefinedType(azslParser::PredefinedTypeContext * /*ctx*/) override { }

  virtual void enterSubobjectType(azslParser::SubobjectTypeContext * /*ctx*/) override { }
  virtual void exitSubobjectType(azslParser::SubobjectTypeContext * /*ctx*/) override { }

  virtual void enterOtherViewResourceType(azslParser::OtherViewResourceTypeContext * /*ctx*/) override { }
  virtual void exitOtherViewResourceType(azslParser::OtherViewResourceTypeContext * /*ctx*/) override { }

  virtual void enterRtxBuiltInTypes(azslParser::RtxBuiltInTypesContext * /*ctx*/) override { }
  virtual void exitRtxBuiltInTypes(azslParser::RtxBuiltInTypesContext * /*ctx*/) override { }

  virtual void enterBufferPredefinedType(azslParser::BufferPredefinedTypeContext * /*ctx*/) override { }
  virtual void exitBufferPredefinedType(azslParser::BufferPredefinedTypeContext * /*ctx*/) override { }

  virtual void enterBufferType(azslParser::BufferTypeContext * /*ctx*/) override { }
  virtual void exitBufferType(azslParser::BufferTypeContext * /*ctx*/) override { }

  virtual void enterByteAddressBufferTypes(azslParser::ByteAddressBufferTypesContext * /*ctx*/) override { }
  virtual void exitByteAddressBufferTypes(azslParser::ByteAddressBufferTypesContext * /*ctx*/) override { }

  virtual void enterPatchPredefinedType(azslParser::PatchPredefinedTypeContext * /*ctx*/) override { }
  virtual void exitPatchPredefinedType(azslParser::PatchPredefinedTypeContext * /*ctx*/) override { }

  virtual void enterPatchType(azslParser::PatchTypeContext * /*ctx*/) override { }
  virtual void exitPatchType(azslParser::PatchTypeContext * /*ctx*/) override { }

  virtual void enterSamplerStatePredefinedType(azslParser::SamplerStatePredefinedTypeContext * /*ctx*/) override { }
  virtual void exitSamplerStatePredefinedType(azslParser::SamplerStatePredefinedTypeContext * /*ctx*/) override { }

  virtual void enterScalarType(azslParser::ScalarTypeContext * /*ctx*/) override { }
  virtual void exitScalarType(azslParser::ScalarTypeContext * /*ctx*/) override { }

  virtual void enterStreamOutputPredefinedType(azslParser::StreamOutputPredefinedTypeContext * /*ctx*/) override { }
  virtual void exitStreamOutputPredefinedType(azslParser::StreamOutputPredefinedTypeContext * /*ctx*/) override { }

  virtual void enterStreamOutputObjectType(azslParser::StreamOutputObjectTypeContext * /*ctx*/) override { }
  virtual void exitStreamOutputObjectType(azslParser::StreamOutputObjectTypeContext * /*ctx*/) override { }

  virtual void enterStructuredBufferPredefinedType(azslParser::StructuredBufferPredefinedTypeContext * /*ctx*/) override { }
  virtual void exitStructuredBufferPredefinedType(azslParser::StructuredBufferPredefinedTypeContext * /*ctx*/) override { }

  virtual void enterStructuredBufferName(azslParser::StructuredBufferNameContext * /*ctx*/) override { }
  virtual void exitStructuredBufferName(azslParser::StructuredBufferNameContext * /*ctx*/) override { }

  virtual void enterTextureType(azslParser::TextureTypeContext * /*ctx*/) override { }
  virtual void exitTextureType(azslParser::TextureTypeContext * /*ctx*/) override { }

  virtual void enterTexturePredefinedType(azslParser::TexturePredefinedTypeContext * /*ctx*/) override { }
  virtual void exitTexturePredefinedType(azslParser::TexturePredefinedTypeContext * /*ctx*/) override { }

  virtual void enterGenericTexturePredefinedType(azslParser::GenericTexturePredefinedTypeContext * /*ctx*/) override { }
  virtual void exitGenericTexturePredefinedType(azslParser::GenericTexturePredefinedTypeContext * /*ctx*/) override { }

  virtual void enterTextureTypeMS(azslParser::TextureTypeMSContext * /*ctx*/) override { }
  virtual void exitTextureTypeMS(azslParser::TextureTypeMSContext * /*ctx*/) override { }

  virtual void enterMsTexturePredefinedType(azslParser::MsTexturePredefinedTypeContext * /*ctx*/) override { }
  virtual void exitMsTexturePredefinedType(azslParser::MsTexturePredefinedTypeContext * /*ctx*/) override { }

  virtual void enterSubpassInputType(azslParser::SubpassInputTypeContext * /*ctx*/) override { }
  virtual void exitSubpassInputType(azslParser::SubpassInputTypeContext * /*ctx*/) override { }

  virtual void enterSubpassInputPredefinedType(azslParser::SubpassInputPredefinedTypeContext * /*ctx*/) override { }
  virtual void exitSubpassInputPredefinedType(azslParser::SubpassInputPredefinedTypeContext * /*ctx*/) override { }

  virtual void enterGenericSubpassInputPredefinedType(azslParser::GenericSubpassInputPredefinedTypeContext * /*ctx*/) override { }
  virtual void exitGenericSubpassInputPredefinedType(azslParser::GenericSubpassInputPredefinedTypeContext * /*ctx*/) override { }

  virtual void enterVectorType(azslParser::VectorTypeContext * /*ctx*/) override { }
  virtual void exitVectorType(azslParser::VectorTypeContext * /*ctx*/) override { }

  virtual void enterGenericVectorType(azslParser::GenericVectorTypeContext * /*ctx*/) override { }
  virtual void exitGenericVectorType(azslParser::GenericVectorTypeContext * /*ctx*/) override { }

  virtual void enterScalarOrVectorType(azslParser::ScalarOrVectorTypeContext * /*ctx*/) override { }
  virtual void exitScalarOrVectorType(azslParser::ScalarOrVectorTypeContext * /*ctx*/) override { }

  virtual void enterScalarOrVectorOrMatrixType(azslParser::ScalarOrVectorOrMatrixTypeContext * /*ctx*/) override { }
  virtual void exitScalarOrVectorOrMatrixType(azslParser::ScalarOrVectorOrMatrixTypeContext * /*ctx*/) override { }

  virtual void enterMatrixType(azslParser::MatrixTypeContext * /*ctx*/) override { }
  virtual void exitMatrixType(azslParser::MatrixTypeContext * /*ctx*/) override { }

  virtual void enterGenericMatrixPredefinedType(azslParser::GenericMatrixPredefinedTypeContext * /*ctx*/) override { }
  virtual void exitGenericMatrixPredefinedType(azslParser::GenericMatrixPredefinedTypeContext * /*ctx*/) override { }

  virtual void enterRegisterAllocation(azslParser::RegisterAllocationContext * /*ctx*/) override { }
  virtual void exitRegisterAllocation(azslParser::RegisterAllocationContext * /*ctx*/) override { }

  virtual void enterSamplerStateProperty(azslParser::SamplerStatePropertyContext * /*ctx*/) override { }
  virtual void exitSamplerStateProperty(azslParser::SamplerStatePropertyContext * /*ctx*/) override { }

  virtual void enterLiteral(azslParser::LiteralContext * /*ctx*/) override { }
  virtual void exitLiteral(azslParser::LiteralContext * /*ctx*/) override { }

  virtual void enterLeadingTypeFunctionSignature(azslParser::LeadingTypeFunctionSignatureContext * /*ctx*/) override { }
  virtual void exitLeadingTypeFunctionSignature(azslParser::LeadingTypeFunctionSignatureContext * /*ctx*/) override { }

  virtual void enterHlslFunctionDefinition(azslParser::HlslFunctionDefinitionContext * /*ctx*/) override { }
  virtual void exitHlslFunctionDefinition(azslParser::HlslFunctionDefinitionContext * /*ctx*/) override { }

  virtual void enterHlslFunctionDeclaration(azslParser::HlslFunctionDeclarationContext * /*ctx*/) override { }
  virtual void exitHlslFunctionDeclaration(azslParser::HlslFunctionDeclarationContext * /*ctx*/) override { }

  virtual void enterUserDefinedType(azslParser::UserDefinedTypeContext * /*ctx*/) override { }
  virtual void exitUserDefinedType(azslParser::UserDefinedTypeContext * /*ctx*/) override { }

  virtual void enterAssociatedTypeDeclaration(azslParser::AssociatedTypeDeclarationContext * /*ctx*/) override { }
  virtual void exitAssociatedTypeDeclaration(azslParser::AssociatedTypeDeclarationContext * /*ctx*/) override { }

  virtual void enterTypedefStatement(azslParser::TypedefStatementContext * /*ctx*/) override { }
  virtual void exitTypedefStatement(azslParser::TypedefStatementContext * /*ctx*/) override { }

  virtual void enterTypealiasStatement(azslParser::TypealiasStatementContext * /*ctx*/) override { }
  virtual void exitTypealiasStatement(azslParser::TypealiasStatementContext * /*ctx*/) override { }

  virtual void enterTypeAliasingDefinitionStatement(azslParser::TypeAliasingDefinitionStatementContext * /*ctx*/) override { }
  virtual void exitTypeAliasingDefinitionStatement(azslParser::TypeAliasingDefinitionStatementContext * /*ctx*/) override { }

  virtual void enterTypeofExpression(azslParser::TypeofExpressionContext * /*ctx*/) override { }
  virtual void exitTypeofExpression(azslParser::TypeofExpressionContext * /*ctx*/) override { }

  virtual void enterGenericParameterList(azslParser::GenericParameterListContext * /*ctx*/) override { }
  virtual void exitGenericParameterList(azslParser::GenericParameterListContext * /*ctx*/) override { }

  virtual void enterGenericTypeDefinition(azslParser::GenericTypeDefinitionContext * /*ctx*/) override { }
  virtual void exitGenericTypeDefinition(azslParser::GenericTypeDefinitionContext * /*ctx*/) override { }

  virtual void enterGenericConstraint(azslParser::GenericConstraintContext * /*ctx*/) override { }
  virtual void exitGenericConstraint(azslParser::GenericConstraintContext * /*ctx*/) override { }

  virtual void enterLanguageDefinedConstraint(azslParser::LanguageDefinedConstraintContext * /*ctx*/) override { }
  virtual void exitLanguageDefinedConstraint(azslParser::LanguageDefinedConstraintContext * /*ctx*/) override { }

  virtual void enterFunctionDeclaration(azslParser::FunctionDeclarationContext * /*ctx*/) override { }
  virtual void exitFunctionDeclaration(azslParser::FunctionDeclarationContext * /*ctx*/) override { }

  virtual void enterAttributedFunctionDeclaration(azslParser::AttributedFunctionDeclarationContext * /*ctx*/) override { }
  virtual void exitAttributedFunctionDeclaration(azslParser::AttributedFunctionDeclarationContext * /*ctx*/) override { }

  virtual void enterFunctionDefinition(azslParser::FunctionDefinitionContext * /*ctx*/) override { }
  virtual void exitFunctionDefinition(azslParser::FunctionDefinitionContext * /*ctx*/) override { }

  virtual void enterAttributedFunctionDefinition(azslParser::AttributedFunctionDefinitionContext * /*ctx*/) override { }
  virtual void exitAttributedFunctionDefinition(azslParser::AttributedFunctionDefinitionContext * /*ctx*/) override { }

  virtual void enterCompilerExtensionStatement(azslParser::CompilerExtensionStatementContext * /*ctx*/) override { }
  virtual void exitCompilerExtensionStatement(azslParser::CompilerExtensionStatementContext * /*ctx*/) override { }

  virtual void enterSrgDefinition(azslParser::SrgDefinitionContext * /*ctx*/) override { }
  virtual void exitSrgDefinition(azslParser::SrgDefinitionContext * /*ctx*/) override { }

  virtual void enterAttributedSrgDefinition(azslParser::AttributedSrgDefinitionContext * /*ctx*/) override { }
  virtual void exitAttributedSrgDefinition(azslParser::AttributedSrgDefinitionContext * /*ctx*/) override { }

  virtual void enterSrgMemberDeclaration(azslParser::SrgMemberDeclarationContext * /*ctx*/) override { }
  virtual void exitSrgMemberDeclaration(azslParser::SrgMemberDeclarationContext * /*ctx*/) override { }

  virtual void enterSrgSemantic(azslParser::SrgSemanticContext * /*ctx*/) override { }
  virtual void exitSrgSemantic(azslParser::SrgSemanticContext * /*ctx*/) override { }

  virtual void enterAttributedSrgSemantic(azslParser::AttributedSrgSemanticContext * /*ctx*/) override { }
  virtual void exitAttributedSrgSemantic(azslParser::AttributedSrgSemanticContext * /*ctx*/) override { }

  virtual void enterSrgSemanticBodyDeclaration(azslParser::SrgSemanticBodyDeclarationContext * /*ctx*/) override { }
  virtual void exitSrgSemanticBodyDeclaration(azslParser::SrgSemanticBodyDeclarationContext * /*ctx*/) override { }

  virtual void enterSrgSemanticMemberDeclaration(azslParser::SrgSemanticMemberDeclarationContext * /*ctx*/) override { }
  virtual void exitSrgSemanticMemberDeclaration(azslParser::SrgSemanticMemberDeclarationContext * /*ctx*/) override { }

  virtual void enterSamplerBodyDeclaration(azslParser::SamplerBodyDeclarationContext * /*ctx*/) override { }
  virtual void exitSamplerBodyDeclaration(azslParser::SamplerBodyDeclarationContext * /*ctx*/) override { }

  virtual void enterSamplerMemberDeclaration(azslParser::SamplerMemberDeclarationContext * /*ctx*/) override { }
  virtual void exitSamplerMemberDeclaration(azslParser::SamplerMemberDeclarationContext * /*ctx*/) override { }

  virtual void enterMaxAnisotropyOption(azslParser::MaxAnisotropyOptionContext * /*ctx*/) override { }
  virtual void exitMaxAnisotropyOption(azslParser::MaxAnisotropyOptionContext * /*ctx*/) override { }

  virtual void enterMinFilterOption(azslParser::MinFilterOptionContext * /*ctx*/) override { }
  virtual void exitMinFilterOption(azslParser::MinFilterOptionContext * /*ctx*/) override { }

  virtual void enterMagFilterOption(azslParser::MagFilterOptionContext * /*ctx*/) override { }
  virtual void exitMagFilterOption(azslParser::MagFilterOptionContext * /*ctx*/) override { }

  virtual void enterMipFilterOption(azslParser::MipFilterOptionContext * /*ctx*/) override { }
  virtual void exitMipFilterOption(azslParser::MipFilterOptionContext * /*ctx*/) override { }

  virtual void enterReductionTypeOption(azslParser::ReductionTypeOptionContext * /*ctx*/) override { }
  virtual void exitReductionTypeOption(azslParser::ReductionTypeOptionContext * /*ctx*/) override { }

  virtual void enterComparisonFunctionOption(azslParser::ComparisonFunctionOptionContext * /*ctx*/) override { }
  virtual void exitComparisonFunctionOption(azslParser::ComparisonFunctionOptionContext * /*ctx*/) override { }

  virtual void enterAddressUOption(azslParser::AddressUOptionContext * /*ctx*/) override { }
  virtual void exitAddressUOption(azslParser::AddressUOptionContext * /*ctx*/) override { }

  virtual void enterAddressVOption(azslParser::AddressVOptionContext * /*ctx*/) override { }
  virtual void exitAddressVOption(azslParser::AddressVOptionContext * /*ctx*/) override { }

  virtual void enterAddressWOption(azslParser::AddressWOptionContext * /*ctx*/) override { }
  virtual void exitAddressWOption(azslParser::AddressWOptionContext * /*ctx*/) override { }

  virtual void enterMinLodOption(azslParser::MinLodOptionContext * /*ctx*/) override { }
  virtual void exitMinLodOption(azslParser::MinLodOptionContext * /*ctx*/) override { }

  virtual void enterMaxLodOption(azslParser::MaxLodOptionContext * /*ctx*/) override { }
  virtual void exitMaxLodOption(azslParser::MaxLodOptionContext * /*ctx*/) override { }

  virtual void enterMipLodBiasOption(azslParser::MipLodBiasOptionContext * /*ctx*/) override { }
  virtual void exitMipLodBiasOption(azslParser::MipLodBiasOptionContext * /*ctx*/) override { }

  virtual void enterBorderColorOption(azslParser::BorderColorOptionContext * /*ctx*/) override { }
  virtual void exitBorderColorOption(azslParser::BorderColorOptionContext * /*ctx*/) override { }

  virtual void enterFilterModeEnum(azslParser::FilterModeEnumContext * /*ctx*/) override { }
  virtual void exitFilterModeEnum(azslParser::FilterModeEnumContext * /*ctx*/) override { }

  virtual void enterReductionTypeEnum(azslParser::ReductionTypeEnumContext * /*ctx*/) override { }
  virtual void exitReductionTypeEnum(azslParser::ReductionTypeEnumContext * /*ctx*/) override { }

  virtual void enterAddressModeEnum(azslParser::AddressModeEnumContext * /*ctx*/) override { }
  virtual void exitAddressModeEnum(azslParser::AddressModeEnumContext * /*ctx*/) override { }

  virtual void enterComparisonFunctionEnum(azslParser::ComparisonFunctionEnumContext * /*ctx*/) override { }
  virtual void exitComparisonFunctionEnum(azslParser::ComparisonFunctionEnumContext * /*ctx*/) override { }

  virtual void enterBorderColorEnum(azslParser::BorderColorEnumContext * /*ctx*/) override { }
  virtual void exitBorderColorEnum(azslParser::BorderColorEnumContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

