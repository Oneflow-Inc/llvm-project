include "prelude.td"

// by template --------------------------------------------------
class BinaryOp <
    string name, string desc, Params params = NoParam
> : Op<
    name, desc,
    ConcatParams<Params<[(In $lhs, $rhs), (Out $res)]>, params>
> {
    let DeviceInferFn = ExternFunction<"BinOpDeviceInfer">;
    let DataTypeInferFn = ExternFunction<"BinOpDataTypeInfer">;
}

def AddOp : BinaryOp<"add", "add two tensors">;
def MinusOp : BinaryOp<"minus", "minus two tensors">;
def DivOp : BinaryOp<"div", "divide two tensors", Params<[(Attr Int:$someArg)]>> {
    let DataTypeInferFn = ExternFunction<"DivOpDataTypeInfer">;
}

def DivModOp : BinaryOp<"divmod", "divide two tensors and give its remainder", Params<[(Out $mod)]>>;

// by let --------------------------------------------------
class BinaryOp2 <string name, string desc> : Op<name, desc> {
    list<dag> ExtendParams = [];
    let Params = [
        (In $lhs, $rhs), 
        (Out $res)
    ] # ExtendParams;
    let DeviceInferFn = ExternFunction<"BinOpDeviceInfer">;
    let DataTypeInferFn = ExternFunction<"BinOpDataTypeInfer">;
}

def AddOp2 : BinaryOp2<"add", "add two tensors">;
def MinusOp2 : BinaryOp2<"minus", "minus two tensors">;
def DivOp2 : BinaryOp2<"div", "divide two tensors"> {
    let ExtendParams = [(Attr Int:$someArg)];
    let DataTypeInferFn = ExternFunction<"DivOpDataTypeInfer">;
}

def DivModOp2 : BinaryOp2<"divmod", "divide two tensors and give its remainder"> {
    let ExtendParams = [(Out $mod)];
}

