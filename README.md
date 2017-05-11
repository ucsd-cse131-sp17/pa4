# Diamondback

![A diamondback](https://upload.wikimedia.org/wikipedia/commons/d/d4/Crotalus_ruber_02.jpg)

[Link to your own repo](https://classroom.github.com/assignment-invitations/27756c353a95da7b957e0d800ca21cc5)

This assignment implements a compiler for the Diamondback language, a small
language with functions, numbers, booleans, and _tuples_.

## Errata

Here are the bugs we found in the provided code after we've released the
assignment.

1. `x < y` and `x > y` is still buggy in the initial release, and it happens
   when `x-y` overflows. If you want, you can use the following code (or merge
   in the latest commit) to fix it:

```
let lbl_thn = gen_temp "prim2_then" in
let lbl_end = gen_temp "prim2_else" in
let op_is   = match o with
  ...
  | Less    -> [ ICmp (Reg(EAX), rhs_loc)
               ; IJl lbl_thn
               ; IMov(Reg EAX, const_false)
               ; IJmp lbl_end
               ; ILabel lbl_thn
               ; IMov(Reg EAX, const_true)
               ; ILabel lbl_end
               ]
  | Greater -> [ ICmp (Reg(EAX), rhs_loc)
               ; IJg lbl_thn
               ; IMov(Reg EAX, const_false)
               ; IJmp lbl_end
               ; ILabel lbl_thn
               ; IMov(Reg EAX, const_true)
               ; ILabel lbl_end
               ]
```

2. Fixed the parser to reject cases where the tuple length is 1 (e.g. `(4,)`)

**Note:** We are **not** going to check your program using test cases that
target the bugs above.

## Language

Diamondback starts with the same semantics as Cobra, and adds support for
tuples.

### Syntax Additions

The main additions in Diamondback are _tuple expressions_. With it an accessor
expression for getting tuple contents (`[ ]`) and a unary primitive for
checking if a value is a tuple (`istuple`). Tuple expressions are a series of
_two or more_ comma-separated expressions enclosed in parentheses.
For example:

```
let x = (3, 4, 5) in
x[0] == 3
```

Finally, `istuple` is a primitive, like `isnum` and `isbool`, but checks for
tuple-ness.

```
expr :=
  [ ... the same expressions as Cobra ... ]
  | (<expr>, <expr>, <expr>, ...)
  | <expr>[<expr>]
  | istuple(<expr>)
```

In the `.ml` these are represented as:

```
type expr =
  ...
  | ETuple of expr list
  | EGetItem of expr * expr

type prim1 =
  ...
  | IsTuple
```

The first `expr` in `EGetItem` is a tuple expression, and the second one is the
expression for the index.

### Semantics and Representation of Tuples

#### Tuple Heap Layout

Tuples expressions should evaluate their sub-expressions in order, and store the
resulting values on the heap. The layout for a tuple on the heap is:

```
 (first 4 bytes)  (4 bytes)  (4 bytes)    ...   (4 bytes)
-----------------------------------------------------------
| # of elements | element_0 | element_1 | ... | element_n |
-----------------------------------------------------------
```

One word is used to store the _number_ of elements, n, in the tuple and the
subsequent words are used to store the values themselves.

A _tuple value_ is stored in variables and registers as **the address of the
first word** in the tuple's memory, but with an additional `1` added to the
value to act as a tag. So, for example, if the start address of the above
memory were `0x0adadad0`, the tuple value would be `0x0adadad1`. With this
change, we extend the set of tag bits to the following:

- Numbers: `0` in least significant bit
- Booleans: `11` in least two significant bits
- Tuples: `01` in least two significant bits

Visualized, the type layout is:

```
0xWWWWWWW[www0] - Number
0xFFFFFFF[1111] - True
0x7FFFFFF[1111] - False
0xWWWWWWW[ww01] - Tuple
```

Where `W` is a "wildcard" nibble and `w` is a "wildcard" bit.

#### Accessing Tuple Contents

In a _tuple access_ expression, like

```
(6, 7, 8, 9)[1 + 2]
```

The behavior should be:

1.  Evaluate the expressions in the tuple (before the brackets), then the
    index expression (the one inside the brackets).
2.  Check that the tuple is actually a tuple by looking for the appropriate tag
    bits, and signal an error containing `"expected a tuple"` if not.
3.  Check that the index value is a number, and signal an error containing
    `"expected a number"` if not.
4.  Check that the index number is a valid index for the tuple value (ie
    between `0` and the stored number of elements in the tuple minus
    one.  Signal an error containing `"index too small"` or `"index too large"`
    as appropriate.
5.  Evaluate to the tuple element at the specified index.

You _can_ do this with just `EAX`, but it causes some minor pain. The register
`ECX` has been added to the registers in `instruction.ml` – feel free to
generate code that uses both `EAX` and `ECX` in this case. This can save a
number of instructions. Make sure that your implementation does not clobber the
value stored in the `ECX` register, before you use it.

You also may want to use an extended syntax for `mov` in order to combine these
values for lookup.  For example, this kind of arithmetic is allowed inside
`mov` instructions:

```
  mov eax, [eax + ecx * 4 + 0]
```

which corresponds to our new `RegOffsetReg` instruction type:

```
RegOffsetReg(EAX, ECX, 4, 0)
```

This would access the memory at the location of `eax`, offset by the value of
`ecx * 4`. So if the value in `ecx` were, say `2`, this may be part of a scheme
for accessing the first element of a tuple (there are other details you should
think through here; this is _not_ a complete solution).

Neither `ECX` nor anything beyond the typical `RegOffset` is _required_ to make
this work, but you may find it interesting to try different shapes of
generated instructions.

#### General Heap Layout

The register `EBX` has been designated as the heap pointer. The provided
`main.c` does a "large" `malloc` call, and passes in the resulting address as an
argument to `our_code_starts_here`. The support code provided fetches this value
(as a traditional argument), and stores it in `EBX`.

It is **up to your code** to ensure that the value of `EBX` is always the
address of the next block of free space (in _increasing_ address order) in the
provided block of memory.

#### Interaction with Existing Features

Any time we add a new feature to a language, we need to consider its
interactions with all the existing features.  In the case of Diamondback, that
means considering:

- If expressions
- Function calls and definitions
- Tuples in binary and unary operators
- Let bindings

We'll take them one at a time.

- **If expressions**: Since we've decided to only allow booleans in conditional
  position, we simply need to make sure our existing checks for boolean-tagged
  values in if continue to work for tuples.
- **Function calls and definitions**: Tuple values behave just like other values
  when passed to and returned from functions – the tuple value is just a
  (tagged) address that takes up a single word.
- **Tuples in let bindings**: As with function calls and returns, tuple values
  take up a single word and act just like other values in let bindings.
- **Tuples in binary operators**: The arithmetic expressions should continue to
  only allow numbers and signal errors on tuple values. There is one binary
  operator that doesn't check its types, however: `==`. We need to decide what
  the behavior of `==` is on two tuple values. Note that we have a (rather
  important) choice here. Clearly, this program should evaluate to `true`:

  ```
  let t = (4, 5) in t == t
  ```

  However, we need to decide if

  ```
  (4,5) == (4,5)
  ```

  should evaluate to `true` or `false`. That is, do we check if the _tuple
  addresses_ are the same to determine equality, or if the _tuple contents_ are
  the same. For this assignment, we'll take the somewhat simpler route and
  compare _addresses_ of tuples, so the second test should evaluate to `false`.
  (If you have extra time on this assignment, it's worth trying out the
  alternate implementation, where you check the tuple contents. A useful hint is
  to write a two-argument function `equal` in `main.c` that handles this. There
  is no extra credit for this, just extra learning, which is immensely more
  valuable.)
- **Tuples in unary operators**: The behavior of the unary operators is
  straightforward, with the exception that we need to implement `print` for
  tuples. We could just print the address, but that would be somewhat
  unsatisfying. Instead, we should recursively print the tuple contents, so that
  the program

  ```
  print((4, (true, 3)))
  ```

  actually prints the string `"(4, (true, 3))"`. This will require some careful
  work with pointers in `main.c`. A useful hint is to create a recursive helper
  function for `print` that traverses the nested structure of tuples and prints
  single values.

## Approaching Reality

With the addition of tuples, Diamondback is dangerously close to a useful
language. Of course, it still puts no control on memory limits, doesn't have a
module system, and has other major holes. However, since we have structured
data, we can now, for instance, implement a linked list. We need to pick a value
to represent `empty` – `false` will do in a pinch. Then we can write `link`,
which creates a pair of the first with the next link:

```
def link(first, rest):
  (first, rest)

let mylist = link(1, link(2, link(3, false))) in
  mylist[0]
```

Now we can write some list functions:

```
def length(l):
  if l == false: 0
  else:
    1 + length(l[1])
```

We've also added a script called `evaluate.sh` that would take a program as an
input, and compile & run it. You can use it to rapidly test your
implementations.

### Optional

Try building on this idea by writing up a basic list library. You can write
`sum` to add up a numeric list, `append` which concatenates two lists, and
`reverse` which reverses a list. Of course, you can also write more interesting
functions and test them out.

## User Inputs

In this assignment, we're introducing a way to get user inputs. This is how it
works:

1. You run the compiled Diamondback code with command line arguments (e.g.
   `./output/foo.run <arg1> <arg2> ...`)
2. Then, access that values inside your code using the `input` function, which
   takes the index of the argument.

For example, assume you have the following program (inside
`input/input_test.diamondback`):

```
def f(i1, i2):
  input(i1) + input(i2)

f(0,0+1)
```

If you compile this program (i.e. `make output/input_test.run`) and run it
as `./output/input_test.run 42 43`, you should get the output `85`.

In `main.c`, there's a function called `input` that handles reading, tagging,
etc. of user arguments. You only have to implement the `Input` case in
`compile_prim1`. Hint: It should be very similar to the implementation of
`Print` from the previous assignment.

## Testing

- You need to add **at least 5 tests** to `myTests.ml`. These will be
automatically evaluated for thoroughness and correctness by checking if they
catch bad implementations.
- You can use `t <name of test> <code> <expected result>` for testing valid
programs.
- To test invalid ones, use `terr <name of test> <code> <expected error
message>`.
- Make sure that each test case has a **unique** name.
- You can use `t_i <name of test> <code> <expected result> <inputs>` and `terr_i
<name of test> <code> <expected error message> <inputs>` if you want to use user
inputs in your test cases.

To recap, these are the possible errors, and their corresponding error messages:

1. Applying `-`, `+`, `*`, `<`, `>`, `add1`, `sub1`, or `input` to a non-number
   argument: `expected a number`
2. Using a non-boolean value for `if`'s condition: `expected a boolean`
3. Overflow occurs when using `-`, `+`, or `*`: `overflow`
4. Unbounded variable: `Variable identifier {id} unbounded`
5. Multiple bindings in a single `let` expression: `Multiple bindings for
   variable identifier {id}`
6. A function application with the wrong number of arguments: `Arity`
7. A function application of a non-existent function: `No such function`
8. A function declaration with duplicate names in the argument list: `Duplicate parameter`
9. Multiple function definitions with the same name: `Duplicate function`
10. If the tuple position of a tuple access is not a tuple: `expected a tuple`
11. If the index position of a tuple access is not a number: `expected a number`
12. If the index position of a tuple access is not within bounds: `index too small` or `index too large`

**Note:** `==` operator should work on _any_ kind of values now.

### Coverage Testing

This time, we're releasing the 7 buggy compiler that we're going to use to grade
your test cases. There's also 1 working (as far as we know) compiler.

To see the output of all these compilers with your test cases:

1. Log into `ieng6`
2. `cd` to the root of your homework (i.e. `pa4-diamondback-<github username>`)
3. Run `pa4_coverage_test <program>` where `<program>` is the string that you
   want to evaluate. If you've written your program into a file, you can use
   `pa4_coverage_test "$(cat <filename>)"`.
4. The corresponding `.s` and `.run` files will be created inside the
   `output{i}` folder, where `{i}` corresponds to the id of the compiler.

## TODO

- `IsNum`, `IsBool` and `IsTuple` in `compile_prim1`. You can use your previous
  implementation from Cobra, if you think the check should be the same.
- `Print` and `Input` in `compile_prim1`. These should call the corresponding
  functions in `main.c` by following the
  [C calling convention](https://www.cs.virginia.edu/~evans/cs216/guides/x86.html#calling):
- `print_simple` in `main.c`: We left out the part of the function that handles
  tuples.
- `compile_equals`: This should check whether the arguments are the same.
  Remember that in this assignment, `==` should work on any type of arguments. If
  you want to implement content equality for tuples, we advise you to implement
  the logic in `main.c` and make a function call from the assembly.
- `ETuple` and `EGetItem` in `well_formed_e`: These should return the _static_
  errors that can arise from tuple and tuple access operations respectively.
- `ETuple` and `EGetItem` in `compile_expr`: These should the tuple and tuple
  access operations respectively.

## FAQ

**Q. Should we keep tuple length unshifted when we store it as the first element of the tuple or should we shift it?**

A. Depends on your implementation. If unshifted, remember to treat it
differently than your regular constants. If shifted, remember to unshift it when
checking for index out of bounds and in `print`. But storing it shifted could be
useful.

**Q. Do all elements of a tuple have to be the same type?**

A. No

**Q. Can tuples be of size 1 or 0?**

A. No

**Q. Why are we using size of `DWord` if element sizes are of size `Word`?**

A. [Assembly is weird](http://www.cs.virginia.edu/~evans/cs216/guides/x86.html)

**Q. Does our max int change from the previous assignment?**

A. Does the number of bits to represent our data change?

**Q. What should happen if we compare two different type of values?**

A. `==` should return false. Here are more examples:

```
1 == 1                  --> true
1 == 2                  --> false
1 == true               --> false
true == true            --> true
(1,2) == 1              --> false
(1,2) == true           --> false
(1,2) == (1,2)          --> false // since the default is address equivalence
let t = (1,2) in t == t --> true
```

## Recommended Ways To Start

1. Get tuple creation and access working for tuples containing two elements, and
   **test** as you go. This is very similar to the pairs code from lecture.

   So, you can just put a `Const(2)` as the first word, use the 2nd and the 3rd
   words for the elements and leave the 4th word blank (to make the 8 byte
   alignment we discussed earlier work).
2. Modify the binary and unary operators to handle tuples appropriately (it may
   be useful to skip `print` at first). **Test** as you go.
3. Make tuple creation and access work for tuples of _any_ size. **Test** as you
   go.
4. Tackle `print` for tuples if you haven't already. **Test** as you go.
5. If you _want_ to try more, implement content-equality rather than
   address-equality for tuples. And/or, try implementing something more
   ambitious than lists, like a binary search tree, in Diamondback. This last
   point is ungraded, but quite rewarding!

A note on support code – a lot is provided, but, as always, you can feel free to overwrite it with your own implementation if you prefer.

