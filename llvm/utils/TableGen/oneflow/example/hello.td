include "prelude.td"

def HelloOp : Op<"hello", "just a demo"> {
    let Params = [
        (In $a, $b),
        (Out $c),
        (Attr Int:$num, VecFloat:$seeds)
    ];
    let DeviceInferFn = DefaultExternFunction;
    let DataTypeInferFn = InlinedFunction<[{
        return Maybe<void>::Ok();
    }]>;
}

