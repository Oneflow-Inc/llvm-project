class ParamNode <string type> {
    string ParamNode = type;
}

def In : ParamNode<"in">;
def Out : ParamNode<"out">;

class TensorType <bit optional> {
    string TensorType = "TensorTuple";
    bit IsOptional = optional;
}

def Tensor : TensorType<0>;
def OptionalTensor : TensorType<1>;

def Attr : ParamNode<"attr">;

class AttributeType <string type> {
    string AttributeType = type;
}

def Int : AttributeType<"int">;
def Float : AttributeType<"float">;

class AttributeVec <AttributeType elemType> : AttributeType<
    !strconcat("std::vector<", elemType.AttributeType, ">")
>;

def VecInt : AttributeVec<Int>;
def VecFloat : AttributeVec<Float>;

// TODO: other types

class Params <
    list<dag> params
> {
    list<dag> Params = params;
}

def NoParam : Params<[]>;

class ConcatParams <
    Params lhs,
    Params rhs
> : Params<lhs.Params # rhs.Params>;

class FunctionMeta <
    string type
> {
    string FunctionMeta = type;
}

def UndefinedFunction : FunctionMeta<"undefined">;

class ExternFunction <string name> : FunctionMeta<"extern"> {
    string FunctionName = name;
}

// if the name is empty, then the default function name `::oneflow::user_op::<OP NAME>::<FN NAME>` is used
def DefaultExternFunction : ExternFunction<"">;

class InlinedFunction <string block> : FunctionMeta<"inlined"> {
    string FunctionCode = block;
}

class Functions {
    FunctionMeta TensorDescInferFn = UndefinedFunction;
    FunctionMeta DeviceInferFn = UndefinedFunction;
    FunctionMeta DataTypeInferFn = UndefinedFunction;
    FunctionMeta GetSbpFn = UndefinedFunction;
    // TODO: other functions
}

class Op <
    string name, 
    string description,
    Params params = NoParam
> : Functions, Params<params.Params> {
    string OpName = name;
    string Description = description;
}

