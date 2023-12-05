/** 
 * SeminalInputFeatureDetector.cpp
 * @author Carter Fultz (cmfultz)
*/

#include "SeminalInputFeatureDetector.h"
#include "KeyPointsCollector.h"

#include <clang-c/Index.h>
#include <iostream>
#include <fstream>
#include <iterator>

SeminalInputFeatureDetector::SeminalInputFeatureDetector( const std::string &filename, bool debug )
    : filename(std::move(filename)), debug(debug) {

    // Get a pointer to the KPC, the cursors obtained from this are need their
    // translation unit to still be active in memory to be able to obtain
    // valid memory reads and prevent heap corruption.
    kpc = new KeyPointsCollector( std::string(filename), false );
    
    // Obtained from part 1, KeyPointsCollector.cpp
    kpc->collectCursors();
    cursorObjs = kpc->getCursorObjs();
    varDecls = kpc->getVarDecls();
    count = 0;

    translationUnit =
        clang_parseTranslationUnit( index, filename.c_str(), nullptr, 0,
                                   nullptr, 0, CXTranslationUnit_None );
    cxFile = clang_getFile( translationUnit, filename.c_str() );
}



CXChildVisitResult SeminalInputFeatureDetector::ifStmtBranch(CXCursor current,
                                                      CXCursor parent,
                                                      CXClientData clientData) {

    // instance of SeminalInputFeatureDetector
    SeminalInputFeatureDetector *instance = static_cast<SeminalInputFeatureDetector *>(clientData);

    if ( !clang_Cursor_isNull( current ) ) {

        // Cursor Type
        CXType cursor_type = clang_getCursorType( current );
        CXString type_spelling = clang_getTypeSpelling( cursor_type );

        // Cursor Location
        CXSourceLocation location = clang_getCursorLocation( current );
        unsigned line;
        clang_getExpansionLocation( location, &instance->cxFile, &line, nullptr, nullptr );
        line += instance->kpc->getNumIncludeDirectives();
        
        // Cursor Token
        CXToken *cursor_token = clang_getToken( instance->kpc->getTU(), location );
        if ( cursor_token ) {
            CXString token_spelling = clang_getTokenSpelling( instance->translationUnit, *cursor_token );

            if ( parent.kind == CXCursor_IfStmt && ( current.kind == CXCursor_UnexposedExpr 
                                                || current.kind == CXCursor_BinaryOperator ) ) {
                if ( instance->debug ) {
                    CXString parent_kind_spelling = clang_getCursorKindSpelling( parent.kind );
                    CXString current_kind_spelling = clang_getCursorKindSpelling( current.kind );

                    std::cout << "  Kind: " << clang_getCString(parent_kind_spelling) << "\n"
                              << "    Kind: " << clang_getCString(current_kind_spelling) << "\n"
                              << "      Type: " << clang_getCString(type_spelling) << "\n"
                              << "      Token: " << clang_getCString(token_spelling) << "\n"
                              << "      Line " << line << "\n\n";

                    clang_disposeString( parent_kind_spelling );
                    clang_disposeString( current_kind_spelling );
                }

                instance->getDeclLocation( clang_getCString(token_spelling), instance->count++, clang_getCString(type_spelling) );
                clang_disposeString( type_spelling );
                clang_disposeString( token_spelling );
                clang_disposeTokens( instance->translationUnit, cursor_token, 1 );
                return CXChildVisit_Break;
            }

            clang_disposeString( token_spelling );
            clang_disposeTokens( instance->translationUnit, cursor_token, 1 );
        }

        clang_disposeString( type_spelling );
    }
    return CXChildVisit_Recurse;
}

CXChildVisitResult SeminalInputFeatureDetector::forStmtBranch(CXCursor current,
                                                      CXCursor parent,
                                                      CXClientData clientData) {

    // instance of SeminalInputFeatureDetector
    SeminalInputFeatureDetector *instance = static_cast<SeminalInputFeatureDetector *>(clientData);

    if ( !clang_Cursor_isNull( current ) ) {
        
        // Cursor Type
        CXType cursor_type = clang_getCursorType( current );
        CXString type_spelling = clang_getTypeSpelling( cursor_type );

        // Cursor Location
        CXSourceLocation location = clang_getCursorLocation( current );
        unsigned line;
        clang_getExpansionLocation( location, &instance->cxFile, &line, nullptr, nullptr );
        line += instance->kpc->getNumIncludeDirectives();

        // Cursor Token
        CXToken *cursor_token = clang_getToken( instance->kpc->getTU(), location );
        if ( cursor_token ) {
            CXString token_spelling = clang_getTokenSpelling( instance->translationUnit, *cursor_token );

            if ( parent.kind == CXCursor_DeclStmt && current.kind == CXCursor_VarDecl ) {
                if ( instance->debug ) {
                    CXString parent_kind_spelling = clang_getCursorKindSpelling( parent.kind );
                    CXString current_kind_spelling = clang_getCursorKindSpelling( current.kind );

                    std::cout << "  Kind: " << clang_getCString(parent_kind_spelling) << "\n"
                              << "    Kind: " << clang_getCString(current_kind_spelling) << "\n"
                              << "      Type: " << clang_getCString(type_spelling) << "\n"
                              << "      Token: " << clang_getCString(token_spelling) << "\n"
                              << "      Line " << line << "\n\n";

                    clang_disposeString( parent_kind_spelling );
                    clang_disposeString( current_kind_spelling );
                }

                instance->temp.name = clang_getCString(token_spelling);
            }
            
            if ( ( parent.kind == CXCursor_BinaryOperator || parent.kind == CXCursor_CallExpr ) && current.kind == CXCursor_UnexposedExpr ) {
                if ( instance->debug ) {
                    CXString parent_kind_spelling = clang_getCursorKindSpelling( parent.kind );
                    CXString current_kind_spelling = clang_getCursorKindSpelling( current.kind );

                    std::cout << "  Kind: " << clang_getCString(parent_kind_spelling) << "\n"
                              << "    Kind: " << clang_getCString(current_kind_spelling) << "\n"
                              << "      Type: " << clang_getCString(type_spelling) << "\n"
                              << "      Token: " << clang_getCString(token_spelling) << "\n"
                              << "      Line " << line << "\n\n";
                    
                    clang_disposeString( parent_kind_spelling );
                    clang_disposeString( current_kind_spelling );
                }

                if ( instance->temp.name != clang_getCString(token_spelling) ) {
                    instance->getDeclLocation( clang_getCString(token_spelling), instance->count++, clang_getCString(type_spelling) );
                    clang_disposeString( type_spelling );
                    clang_disposeString( token_spelling );
                    clang_disposeTokens( instance->translationUnit, cursor_token, 1 );
                    return CXChildVisit_Break;
                }
            }
            
            clang_disposeString( token_spelling );
            clang_disposeTokens( instance->translationUnit, cursor_token, 1 );
        }

        clang_disposeString( type_spelling );
    }
    return CXChildVisit_Recurse;
}

CXChildVisitResult SeminalInputFeatureDetector::whileStmtBranch(CXCursor current,
                                                      CXCursor parent,
                                                      CXClientData clientData) {

    // instance of SeminalInputFeatureDetector
    SeminalInputFeatureDetector *instance = static_cast<SeminalInputFeatureDetector *>(clientData);

    if ( !clang_Cursor_isNull( current ) ) {

        // Cursor Type
        CXType cursor_type = clang_getCursorType( current );
        CXString type_spelling = clang_getTypeSpelling( cursor_type );

        // Cursor Location
        CXSourceLocation location = clang_getCursorLocation( current );
        unsigned line;
        clang_getExpansionLocation( location, &instance->cxFile, &line, nullptr, nullptr );
        line += instance->kpc->getNumIncludeDirectives();
        
        // Check for break statements
        if ( parent.kind == CXCursor_IfStmt && current.kind == CXCursor_BreakStmt ) {
            clang_visitChildren( parent, instance->ifStmtBranch, instance );

        } else if ( ( parent.kind == CXCursor_BinaryOperator || parent.kind == CXCursor_CallExpr ) && current.kind == CXCursor_UnexposedExpr ) {
            // Cursor Token
            CXToken *cursor_token = clang_getToken( instance->kpc->getTU(), location );
            if ( cursor_token ) {
                CXString token_spelling = clang_getTokenSpelling( instance->kpc->getTU(), *cursor_token );
                if ( instance->debug ) {
                    // Cursor Kind
                    CXString parent_kind_spelling = clang_getCursorKindSpelling( parent.kind );
                    CXString current_kind_spelling = clang_getCursorKindSpelling( current.kind );

                    std::cout << "  Kind: " << clang_getCString(parent_kind_spelling) << "\n"
                            << "    Kind: " << clang_getCString(current_kind_spelling) << "\n"
                            << "      Type: " << clang_getCString(type_spelling) << "\n"
                            << "      Token: " << clang_getCString(token_spelling) << "\n"
                            << "      Line " << line << "\n\n";

                    clang_disposeString( parent_kind_spelling );
                    clang_disposeString( current_kind_spelling );
                }

                instance->getDeclLocation( clang_getCString(token_spelling), instance->count++, clang_getCString(type_spelling) );
                clang_disposeString( token_spelling );
                clang_disposeTokens( instance->translationUnit, cursor_token, 1 );
            }
        }

        clang_disposeString( type_spelling );
    }
    return CXChildVisit_Recurse;
}



void SeminalInputFeatureDetector::getDeclLocation( std::string name, int index, std::string type ) {
    
    // Check if variable already exists in the vector of SeminalInputFeatures
    bool exists = false;
    for ( int i = 0; i < SeminalInputFeatures.size(); i++ ) {
        if ( SeminalInputFeatures[ i ].name == name ) {
            exists = true;
        }
    }

    // Check if variable exists in the map of variable declarations
    std::map<std::string, unsigned>::iterator it;
    it = varDecls.find( name );
    if ( !exists ) {
        if ( it != varDecls.end() ) {
            temp.name = it->first;
            temp.line = it->second;
            temp.type = type;
            SeminalInputFeatures.push_back( temp );
        } else if ( debug ) {
            std::cout << "Variable was not found.\n\n";
        }
    } else if ( debug ) {
        std::cout << "Variable is already accounted for.\n\n";
    }
}

void SeminalInputFeatureDetector::printSeminalInputFeatures() {
    for ( int i = 0; i < SeminalInputFeatures.size(); i++ ) {
        if ( SeminalInputFeatures[ i ].type == "FILE *" ) {
            std::cout << "Line " << SeminalInputFeatures[ i ].line << ": size of file "
                  << SeminalInputFeatures[ i ].name << "\n";
        } else {
            std::cout << "Line " << SeminalInputFeatures[ i ].line << ": "
                  << SeminalInputFeatures[ i ].name << "\n";
        }
    }
}

void SeminalInputFeatureDetector::cursorFinder() {

    // DEBUGGING: printing out list of variable declarations
    if ( debug ) {
        std::cout << "Variable Declarations: \n";
        for( const std::pair<std::string, unsigned> var : varDecls ) {
            std::cout << var.second << ": " << var.first << "\n";
        }
        std::cout << "\n";
    }

    // Looks at each of the cursor objects to recursively search through
    for ( int i = 0; i < cursorObjs.size(); i++ ) {

        if ( !clang_Cursor_isNull( cursorObjs[i] ) ) {
            if ( debug ) {
                CXString kind_spelling = clang_getCursorKindSpelling( cursorObjs[i].kind );
                std::cout << "Kind: " << clang_getCString(kind_spelling) << "\n";
                clang_disposeString( kind_spelling );
            }

            switch ( cursorObjs[i].kind ) {
                case CXCursor_IfStmt:
                    clang_visitChildren( cursorObjs[i], this->ifStmtBranch, this );
                    break;
                case CXCursor_ForStmt:
                    clang_visitChildren( cursorObjs[i], this->forStmtBranch, this );
                    break;
                case CXCursor_WhileStmt:
                    clang_visitChildren( cursorObjs[i], this->whileStmtBranch, this );
                    break;
                default:
                    std::cout << "am i dumb af?" << cursorObjs[i].kind << "\n";
                    break;
            }

            if ( debug ) {
                std::cout << "\n";
            }
        }
    }

    clang_disposeTranslationUnit( translationUnit );
    clang_disposeIndex( index );
    delete kpc;

    printSeminalInputFeatures();
}
