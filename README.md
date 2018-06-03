# Zero Preprocessor

A cross-platform cross-compiler source to source translator that will allow us to play around with some new compile time language features such as Static Reflection and Meta Classes.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for testing purposes.

### Prerequisites

A prerequisite for this project is Boost 1.67

### Installing

For now all you need to do is git clone the repo, add it as a sub_directory and call the function on the target you wish to include the translator for.
```
# add our preprocessor
set(preprocessor_dir "path/to/cloned/repo")
# where to build it relative to your project build dir or an absolute path
set(preprocessor_build_dir "./preprocessor_build")
add_subdirectory(${preprocessor_dir} ${preprocessor_build_dir} EXCLUDE_FROM_ALL)
# preprocess our example target
preprocess(example ${preprocessor_dir})
```

## Examples

Full examples of usages of the implemented features are located in the examples folder.

## Versioning

The project does not use versioning for now, as you should always build from the master branch.

## TODOs
#### Parser:
- [x] variables
- [x] expressions
- [x] functions
- [x] struct/class
- [x] methods and data members
- [ ] virtual/overload methods
- [x] type templates
- [x] non-type templates
- [ ] variadic templates
- [x] Inheritance
- [ ] namespaces
- [ ] enum/union
- [ ] lambdas
- [ ] structured binding declaration
- [ ] functions with -> return type

#### Meta Classes
- [x] Parsing of meta classes in code
- [x] generator -> in constexpr function
- [x] get class functions
- [x] -> variable$ in {}
- [x] multiple files
- [x] -> with target function
- [x] get class variables
- [x] compiler.require
- [ ] method modifier status i.g. is_public()
- [ ] constructors/destructors status


#### Static Reflection
- [x] reflexpr<>
- [x] reflect::get_name
- [x] reflect::get_public_data_members
- [x] reflect::get_size and reflect::get_element
- [x] reflect::get_pointer (for data members)
- [ ] reflect::get_data_members
- [x] reflect::get_public_base_classes
- [ ] reflect::get_base_classes
- [ ] reflect::is_final
- [ ] enum operations
- [ ] reflect::is_virtual for methods
- [ ] reflexpr() for variables and namespaces
- [ ] reflect::is_inline for namespaces

## Contributing

There are TODO comments throughout the project. Before starting to fix one please open an issue referencing the TODO in code.

## License

This project is licensed under the Apache License 2.0 License - see the [LICENSE.md](LICENSE.md) file for details
