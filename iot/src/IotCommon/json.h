// ------------------------------------------------------------------
// JSON stream parser - include file
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------
//
// Supported JSON syntax:
//
//    <sign>       + | -
//    <integer>    <sign> + [0-9]
//    <string>     " + [all chars, \ escaped] + "
//    <array>      [ + <namval> + ( , <namval> )* + ]
//    <object>     { + <namval> + ( , <namval> )* + }
//    <value>      <integer> | <string> | <array> | <object>
//    <namval>     <name> : <value>
//    <json>       ( <namval> | <object> )*
//
// ------------------------------------------------------------------
//
// Example:
//    "sensor" : [ "t" : -23000, "h" : +35 ]
//               ^             ^          ^^
//               1             2          34
//
// 1: '[' indicates the start of an array, onArrayStart( "sensor" ) is called
// 2: ',' indicates the end of an integer name/value pair. 
//           onInteger( "t", -23000 ) is called
// 3: ' ' indicates the end of an integer name/value pair. 
//           onInteger( "h", 35 ) is called
// 4: ']' indicates the end of an array, onArrayComplete( "sensor" ) is called
//
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// Interface
// ------------------------------------------------------------------

typedef void (*OnError)( int error, char * errtext, char * lastchars );
typedef void (*OnObjectStart)( char * name );
typedef void (*OnObjectComplete)( char * name );
typedef void (*OnArrayStart)( char * name );
typedef void (*OnArrayComplete)( char * name );
typedef void (*OnString)( char * name, char * value );
typedef void (*OnInteger)( char * name, int value );

// ------------------------------------------------------------------
// User functions
// ------------------------------------------------------------------

void jsonReset( void );
void jsonEat( char c );
void jsonSetOnError( OnError oe );
void jsonSetOnObjectStart( OnObjectStart oos );
void jsonSetOnObjectComplete( OnObjectComplete oo );
void jsonSetOnArrayStart( OnArrayStart oas );
void jsonSetOnArrayComplete( OnArrayComplete oa );
void jsonSetOnString( OnString os );
void jsonSetOnInteger( OnInteger oi );

// ------------------------------------------------------------------
// Parser switching (when needing multiple parsers in parallel)
// ------------------------------------------------------------------

void jsonSelect( int p );
void jsonSelectNext( void );
void jsonSelectPrev( void );

