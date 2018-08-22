// Copyright (c) 2018, Transnat Games
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

var DC_JS_strings = [];
var DC_JS_functions = [];

var DC_JS_function_prefix = "s=[];var u=0.0;";
var DC_JS_function_suffix = "return s[0];";

function DC_JS_FindFirstFreeSlot(array){
    // Search for freed string slots
    var func = function(obj){ return typeof obj === "object"; }
    var i = array.find(func);
    if(i != -1)
        return i;
    
    DC_JS_strings.push(null);
    return DC_JS_strings.length() - 1;
}

function DC_JS_CreateCalculationBuilder(){
    var string_num = DC_JS_FindFirstFreeSlot(DC_JS_strings);
    DC_JS_strings[string_num] = DC_JS_function_prefix;
    return string_num;
}

function DC_JS_BuildPushImm(string_num, arg_num, immediate){
    DC_JS_strings[string_num] += "s.push("+immediate+");";
}

function DC_JS_BuildPushArg(string_num, arg_num){
    DC_JS_BuildPushImm("a["+arg_num+"]");
}

function DC_JS_BuildOperator(string_num, operator){
    DC_JS_BuildPop(string_num);
    DC_JS_BuildOperatorImm(string_num, operator, "u");
}


function DC_JS_BuildPop(string_num){
    DC_JS_strings[string_num] += "u=s.pop();";
}

function DC_JS_BuildMathBuiltinImm(string_num, name, immediate){
    DC_JS_strings[string_num] += "s.push(Math."+name+"("+immediate+"));";
}

function DC_JS_BuildMathBuiltinArg(string_num, name, arg_num){
    DC_JS_BuildMathBuiltinImm(string_num, name, "a["+arg_num+"]");
}

function DC_JS_BuildMathBuiltin(string_num, name){
    DC_JS_BuildMathBuiltinImm(string_num, name, "s.pop()");
}

function DC_JS_AbandonCalculation(string_num){
    DC_JS_strings[string_num] = null;
}

function DC_JS_BuildOperatorArg(string_num, operator, arg_num){
    DC_JS_BuildOperatorImm(string_num, operator, "a["+arg_num+"]");
}

function DC_JS_BuildOperatorImm(string_num, operator, immediate){
    DC_JS_strings[string_num] += "s.push(s.pop()"+operator+""+immediate+");";
}

function DC_JS_FinalizeCalculation(string_num){
    var function_num = DC_JS_FindFirstFreeSlot(DC_JS_functions);
    DC_JS_functions[function_num] = new Function("a", DC_JS_strings[string_num]);
    DC_JS_AbandonCalculation(string_num);
    return function_num;
}

function DC_JS_Free(function_num){
    DC_JS_functions[function_num] = null;
}

var DC_JS_args = [];

function DC_JS_InitArgs(){
    DC_JS_args = [];
}

function DC_JS_AppendArg(a){
    DC_JS_args.push(a);
}

function DC_JS_Calculate(function_num){
    return DC_JS_functions[function_num](DC_JS_args);
}
