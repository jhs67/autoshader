autoshader
==========

This aims to be a tool to automate some of the tasks needed to use vulkan
shaders from c++ code.

There are two facets to the autoshader package. The first is a tool that
analysis compiled spirv code to create a c++ reflection of the pipeline
interface. This includes reflection of the structure types used in uniform and
storage buffers and also information about the sets and bindings used for
uniform and storage variables. Information about vertex input locations and
types can also be reflected for graphics pipelines.

The second is a header only library that uses the reflected interface
information to create interfaces you can use to create and interact with
the pipeline.


Type reflection
---------------

The autoshader program reflects the structures used in the pipeline bindings
into c structures. The memory layout of the reflected types should match the
corresponding types in the shader. Padding structure members will be added to
ensure the memory layout matches.

The autoshader program assumes there are a c++ types matching the glsl matrix
and vector types (vec3, ivec2, mat2x4, etc.). The autoshader program also
assumes that these types will be tightly packed in memory. The program is
tested with [glm](https://glm.g-truc.net/) and it's use is recommended.

There are two cases where things can be difficult to reflect. Spirv allows for
arrays and matrix columns to have strides that are larger than the underlying
types. The autoshader program will create unnamed structures to hold the value
and padding together. An example will make things clear. If you had this block
in your shader:

```
layout(std140) uniform Uniform {
	float f[10];
	mat2 m;
};
```

This would be reflected in c++ like this:

```
struct Uniform {
	struct { float v; float p0, p1, p2; } f[10];
	struct { vec2 v; float p0, p1; } m[2];
};
```

And you would access the values from c++ with `Uniform u; u.f[0].v = ...`. It
is an inelegant but light weight and simple solution to a difficult problem.

Generally, the autoshader program will reflect the type using the name from
the shader program. It is possible to have duplicate names if a structure is
declared differently in different shader stages or used as a member of buffers
with differing alignment rules. If a mismatching declaration ambiguity occurs
the names are made unique by appending and underscore and the shader stage
name (_comp, _frag, etc.). For alignment variations an underscore and integer
will be appended (_0, _1, etc.) and the names will be ordered from the
smallest structure size to the largest.
