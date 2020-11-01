#include <QCoreApplication>
#include <QFile>
#include <QString>
#include <QDebug>

#include <iostream>

#include <cppast/code_generator.hpp>         // for generate_code()
#include <cppast/cpp_entity_kind.hpp>        // for the cpp_entity_kind definition
#include <cppast/cpp_forward_declarable.hpp> // for is_definition()
#include <cppast/cpp_namespace.hpp>          // for cpp_namespace
#include <cppast/cpp_type.hpp>
#include <cppast/cpp_member_function.hpp>
#include <cppast/libclang_parser.hpp> // for libclang_parser, libclang_compile_config, cpp_entity,...
#include <cppast/visitor.hpp>         // for visit()


using namespace std;


enum class ParamType
{
  Unknown = 0,
  JSCompatible, // bool, int, string, etc.  (too general??)

  Boolean,

  Pointer, // set raw pointer to raw data
  Object, // class, struct, etc.
};


class Parameter
{
public:
  QString mName;
  QString mType;
  ParamType paramType;// = ParamType::Unknown;
};


class FunctionBase
{
public:
  QVector<Parameter> mParams;
};


class Function : public FunctionBase
{
public:
  QString mName;
  QString mReturnTypeString;
  ParamType mReturnType = ParamType::Unknown;
};


class MemberFunction : public Function
{
public:

};


class StaticFunction : public Function
{
public:

};


class Ctor : public FunctionBase
{

};


class ClassDef
{
public:
  bool mValid = false;
  QString mName;
  QVector<Ctor> mCtors;
  QVector<MemberFunction> mMemberFunctions;
  QVector<StaticFunction> mStaticFunctions;
};


void getReturnType(Function &function, const cppast::cpp_type &returnType)
{
  switch (returnType.kind())
  {
    case cppast::cpp_type_kind::builtin_t:
    {
      auto& builtin = static_cast<const cppast::cpp_builtin_type &>(returnType);

      switch (builtin.builtin_type_kind())
      {
        case cppast::cpp_builtin_type_kind::cpp_bool:
        {
          function.mReturnType = ParamType::Boolean;
          function.mReturnTypeString = "bool";
          break;
        }

        default:
        {
          function.mReturnTypeString = "builtinoops";
          break;
        }
      }


      break;
    }

    case cppast::cpp_type_kind::pointer_t:
    {
      function.mReturnType = ParamType::Pointer;
      function.mReturnTypeString = "auto *";
      break;
    }

    default:
    {
      function.mReturnTypeString = "defaultoops";
      break;
    }
  }
}


// TODO: accept cpp_function_base instead of params
void getFunctionParameters(FunctionBase &function, cppast::detail::iteratable_intrusive_list<cppast::cpp_function_parameter> params)
{
  std::for_each(params.begin(), params.end(), [&function](const cppast::cpp_function_parameter &param)
  {
    QString paramName(QString::fromStdString(param.name()));
    //qWarning() << "param type int" << (int)param.type().kind() << "for param with name" << paramName;

    QString typeString;
    ParamType type = ParamType::Unknown;
    switch (param.type().kind())
    {
      case cppast::cpp_type_kind::builtin_t:
      {
        auto& builtin = static_cast<const cppast::cpp_builtin_type &>(param.type());
        type = ParamType::JSCompatible;

        switch (builtin.builtin_type_kind())
        {
          case cppast::cpp_builtin_type_kind::cpp_bool:
          {
            typeString = "bool";
            type = ParamType::Boolean;
            break;
          }

          // etc....

          default:
          {
            typeString = "auto /* built-in unknown */"; // oops, try to hide the failure :)
            break;
          }
        }

        break;
      }

      case cppast::cpp_type_kind::pointer_t:
      {
        //std::cerr << "POINTER KIND: " << (int)param.kind() << std::endl;
        //std::cerr << "POINTER TYPE KIND: " << (int)param.type().kind() << std::endl;

        auto& pointer = static_cast<const cppast::cpp_pointer_type &>(param.type());
        //std::cerr << "POINTER POINTEE KIND: " << (int)pointer.pointee().kind() << std::endl;

        if (pointer.pointee().kind() == cppast::cpp_type_kind::builtin_t)
        {
          auto& builtin = static_cast<const cppast::cpp_builtin_type &>(pointer.pointee());
          // cpp_builtin_type_kind \/
          typeString = QString::fromUtf8(cppast::to_string(builtin.builtin_type_kind()));
          //std::cerr << "builtin => " << typeString.toStdString().c_str() << "(" << (int)builtin.builtin_type_kind() << ")" << std::endl;

        }
//        else ( pointer.pointee().kind() == cppast::cpp_type_kind::unexposed_t )
//      {
//    cpp_unexposed_type
//    }



        //pointer.kind()


        typeString = QString("%1 *").arg(typeString);
        //typeString = "auto *"; // TODO: un-auto
        type = ParamType::Pointer;
        break;
      }

      default:
      {
        typeString = "auto /* unknown */"; // hide our failure
        type = ParamType::Object; // ???
        break;
      }
    }


    function.mParams += { paramName, typeString, type };
    //qWarning() << "parameter" << paramName << "is of type" << typeString;
  });

  //qWarning() << "???" << function.mParams.count();
}


QString readFile(const QString &filename)
{
  QFile f(":/templates/" + filename);
  if (!f.open(QIODevice::ReadOnly))
  {
    cerr << "cannot open resource file '" << filename.toStdString() << "': " << f.errorString().toStdString() << endl;
    abort();
  }
  return f.readAll();
}


int main(int argc, char **argv)
{
  Q_INIT_RESOURCE(res);
  QCoreApplication app(argc, argv);

  cppast::libclang_compile_config config;

  const QStringList args(app.arguments());

  if (args.count() < 2)
  {
    qWarning() << "usage:" << args.at(0) << "<header files> -O <output file> [-I <include paths ...>)] [-D <definitions ...>]";
    return -1;
  }

  enum class ArgType
  {
    Skip,
    Source,
    Include,
    Definition,
    Output,
  };

  QStringList sourceFiles;
  QStringList includes;
  QString output;
  static QStringList paramSwitches({ "-I", "-D", "-O" });
  ArgType argType = ArgType::Skip;
  for (const QString &arg : args)
  {
    qWarning() << "param" << arg;

    switch (paramSwitches.indexOf(arg))
    {
      case 0:
      {
        argType = ArgType::Include;
        continue;
      }

      case 1:
      {
        argType = ArgType::Definition;
        continue;
      }

      case 2:
      {
        argType = ArgType::Output;
        continue;
      }
    }

    switch (argType)
    {
      case ArgType::Skip:
      {
        //qWarning() << "(skipped)";
        argType = ArgType::Source;
        continue;
      }

      case ArgType::Source:
      {
        //qWarning() << "(added as source)";

        if (arg.endsWith(".h") || args.endsWith(".hpp") || args.endsWith(".hxx"))
          sourceFiles += arg;
        else
          qWarning() << "(not adding non-header file" << arg << ")";

        break;
      }

      case ArgType::Output:
      {
        //qWarning() << "(set as output)";
        output = arg;
        break;
      }

      case ArgType::Include:
      {
        includes = arg.split(";", Qt::SkipEmptyParts);
        for (const QString &include : qAsConst(includes))
        {
          if (include.isEmpty())
            break;

          cerr << "include: " << include.toStdString() << endl;
          //qWarning() << "(added as include) [" << include << "]";
          config.add_include_dir(include.toStdString());
        }
        break;
      }

      case ArgType::Definition:
      {
        QStringList defs(arg.split(";"));
        for (const QString &def : qAsConst(defs))
        {
          QStringList x(def.split("="));
          if (x.count() == 1)
            config.define_macro(x.at(0).toStdString(), {});
          else if (x.count() == 2)
            config.define_macro(x.at(0).toStdString(), x.at(1).toStdString());

          //qWarning() << "(added as definition) [" << x << "]";
        }

        break;
      }
    }
  }


  if (sourceFiles.isEmpty())
  {
    qWarning() << "no source files given";
    return -1;
  }


  if (output.isEmpty())
  {
    qWarning() << "no output given";
    return -1;
  }


  cppast::compile_flags flags;
  config.set_flags(cppast::cpp_standard::cpp_11, flags);

  flags |= cppast::compile_flag::gnu_extensions;

  config.enable_feature("PIC");
  config.enable_feature("diagnostics-format=clang");

  cppast::stderr_diagnostic_logger logger;
  logger.set_verbose(true);



  //auto file = parse_file(config, logger, args.at(1), 1);

  cppast::cpp_entity_index idx;
  // the parser is used to parse the entity
  // there can be multiple parser implementations
  cppast::libclang_parser parser(type_safe::ref(logger));



  QMap<QString, ClassDef> classDefs;



  for (const QString &filename : qAsConst(sourceFiles))
  {
    //qWarning() << "parsing file" << filename;

    // parse the file
    auto file = parser.parse(idx, filename.toStdString(), config);
    if (parser.error())
    {
      qDebug() << "parser error";
      return -1;
    }


    ClassDef currentClass;
    QVector<Function> functions;


    cppast::visit(static_cast<const cppast::cpp_file &>(*file), [&](const cppast::cpp_entity& e, cppast::visitor_info info)
    {
      qWarning() << "visiting entity" << QString::fromStdString(e.name()) << "kind =" << (int)e.kind();



//      if (e.parent().has_value())
//        qWarning() << "parent" << QString::fromStdString(e.parent().value().name());

//                 << "enter?" << (info.event == cppast::visitor_info::container_entity_enter)
//                 << "exit?" << (info.event == cppast::visitor_info::container_entity_exit);

      if (e.kind() == cppast::cpp_entity_kind::file_t || cppast::is_templated(e) || cppast::is_friended(e))
        // no need to do anything for a file,
        // templated and friended entities are just proxies, so skip those as well
        // return true to continue visit for children
        return true;

//      switch (e.kind())
//      {
//        case cppast::cpp_entity_kind::include_directive_t:
//          return false;
//      }

      // cppast::cpp_file   ..... ka.... check only THIS file !!

      static QStringList ignoreFunctions({ "metaObject", "qt_metacast", "staticMetaObject", "tr", "trUtf8", "qt_static_metacall" });


      if (currentClass.mValid && info.event == cppast::visitor_info::container_entity_exit)
      {
        qWarning() << "class def for"<<currentClass.mName<<"done!";
        classDefs.insert(currentClass.mName, currentClass);
        currentClass = {};
      }


      if (e.kind() == cppast::cpp_entity_kind::class_t && info.event == cppast::visitor_info::container_entity_enter && !currentClass.mValid)
      {
        auto& _class = static_cast<const cppast::cpp_class &>(e);

        currentClass.mValid = true;
        currentClass.mName = QString::fromStdString(_class.name());
        qWarning() << "new class" << currentClass.mName;
      }
      else if (e.kind() == cppast::cpp_entity_kind::constructor_t && currentClass.mValid)
      {
        cerr << "ctor for class " << currentClass.mName.toStdString() << endl;

        auto &ctor = static_cast<const cppast::cpp_constructor &>(e);

        Ctor _ctor;
        getFunctionParameters(_ctor, ctor.parameters());

        currentClass.mCtors += _ctor;
      }
      else if (e.kind() == cppast::cpp_entity_kind::member_function_t && currentClass.mValid)
      {
        QString memberFunctionName(QString::fromStdString(e.name()));

        qWarning() << "member function"<<memberFunctionName<<"for class" << currentClass.mName;

        if (ignoreFunctions.indexOf(memberFunctionName) != -1)
        {
          qWarning() << "(ignoring)";
          return true;
        }

        auto &member = static_cast<const cppast::cpp_member_function&>(e);

        MemberFunction memberFunction;
        getFunctionParameters(memberFunction, member.parameters());
        getReturnType(memberFunction, member.return_type());
        memberFunction.mName = memberFunctionName;

        currentClass.mMemberFunctions += memberFunction;
      }
      else if (e.kind() == cppast::cpp_entity_kind::function_t && currentClass.mValid) // class static
      {
        QString staticFunctionName(QString::fromStdString(e.name()));

        qWarning() << "static function"<<staticFunctionName<<"for class" << currentClass.mName;

        if (ignoreFunctions.indexOf(staticFunctionName) != -1)
        {
          qWarning() << "(ignoring)";
          return true;
        }

        auto &_static = static_cast<const cppast::cpp_function &>(e);

        StaticFunction staticFunction;
        getFunctionParameters(staticFunction, _static.parameters());
        getReturnType(staticFunction, _static.return_type());
        staticFunction.mName = staticFunctionName;

        currentClass.mStaticFunctions += staticFunction;

        //currentClass.mStaticFunctions += { QString::fromStdString(e.name()), {} };
      }
      else if (e.kind() == cppast::cpp_entity_kind::function_t && !currentClass.mValid) // function (outside of class)
      {
        if (e.parent() && e.parent().value().kind() != cppast::cpp_entity_kind::class_t) // because of cpp files
        {
          QString functionName(QString::fromStdString(e.name()));

          qWarning() << "function"<<functionName;

          auto &_function = static_cast<const cppast::cpp_function &>(e);

          Function function;
          getFunctionParameters(function, _function.parameters());

          qWarning() << "param count" << function.mParams.count();

          getReturnType(function, _function.return_type());
          function.mName = functionName;

          functions += function;
        }
      }



  //      out << " >>MEMBER FUNCTION T!!<< ";

  //      auto& mfun = static_cast<const cppast::cpp_member_function&>(e);

  //      out << " fn name: >>" << mfun.name() << "<< ";
  //      out << " return type int: >>" << (int)mfun.return_type().kind() << "<< ";


  //      if (mfun.return_type().kind() == cppast::cpp_type_kind::unexposed_t)
  //      {
  //        auto& type = static_cast<const cppast::cpp_unexposed_type&>(mfun.return_type());

  //        out << " return type: >>" << type.name() << "<< ";
  //      }


  //      if (mfun.return_type().kind() == cppast::cpp_type_kind::pointer_t)
  //      {
  //        auto& type = static_cast<const cppast::cpp_pointer_type&>(mfun.return_type());
  //        out << " pointer return type int: >>" << (int)type.kind() << "<< ";

  //        auto& type2 = static_cast<const cppast::cpp_pointer_type&>(type.pointee());
  //        out << " pointer return type int 2: >>" << (int)type2.kind() << "<< ";


  //        if (type2.kind() == cppast::cpp_type_kind::user_defined_t)
  //        {
  //          auto& type3 = static_cast<const cppast::cpp_user_defined_type&>(static_cast<const cppast::cpp_type&>(type2));
  //          out << " class return type: >>" << type3.entity().name() << "<<";
  //  //          out << " class return type int: >>" << (int)type3.kind() << "<< ";
  //        }

  //        //if (type.pointee() == cppast::cpp_type_kind:)

  //        //out << " pointer return type: >>" << type.name() << "<< ";
  //      }


  //      if (mfun.return_type().kind() == cppast::cpp_type_kind::builtin_t)
  //      {
  //        auto& type = static_cast<const cppast::cpp_builtin_type&>(mfun.return_type());

  //        out << " built-in return type: >>" << cppast::to_string(type.builtin_type_kind()) << "<< ";
  //      }


  //    }



      return true;

    });





    // init
    QString content;

    // > functions
    for(const Function &f : qAsConst(functions))
    {
      content += readFile("function.tpl").arg(f.mName).arg(f.mName).arg("glob_obj");
    }


#define s QString

    // > classes
    for (const ClassDef &c : qAsConst(classDefs))
    {
//      if (c.mStaticFunctions.isEmpty()) // no need for global definitions for nen-static members
//        continue;

      QString classHandler;
      const QString &className(c.mName);

      qWarning() << "handler for class" << className;

      classHandler += "\n";
      classHandler += "  // class\n";
      classHandler += "  {\n"; // scoped class
      classHandler += "    auto classObj = jerry_create_object();\n\n";

      //int ctorn = 0;
      // TODO: don't create ctor if ctor deleted or private
      for (int ctorn = 0; (ctorn < c.mCtors.count() || ctorn == 0) /* at least one ctor! */; ctorn++)//const Ctor &ctor : qAsConst(c.mCtors))
      {
        classHandler += s("    {\n");
        classHandler += s("      auto ctor = jerry_create_external_function(_rtjs_%1_ctor%2_handler);\n").arg(className).arg(ctorn);
        classHandler += s("      auto ctorName = jerry_create_string((const jerry_char_t *)\"ctor%1\");\n").arg(ctorn);
        classHandler += s("      auto prop = jerry_set_property(classObj, ctorName, ctor);\n");
        classHandler += s("    }\n");

        //ctorn++;
      }

      for (const StaticFunction &m : qAsConst(c.mStaticFunctions))
      {
        const QString staticFunctionName(m.mName);

        classHandler += s("    {\n");
        classHandler += s("      auto staticFunction = jerry_create_external_function(_rtjs_%1_%2_handler);\n").arg(className, staticFunctionName);
        classHandler += s("      auto staticFunctionName = jerry_create_string((const jerry_char_t *)\"%1\");\n").arg(staticFunctionName);
        classHandler += s("      auto prop = jerry_set_property(classObj, staticFunctionName, staticFunction);\n");
        classHandler += s("    }\n");
      }

      classHandler += s("\n");
      classHandler += s("    auto classObjName = jerry_create_string((const jerry_char_t *)\"%1\");\n").arg(className);
      classHandler += s("    jerry_set_property(glob_obj, classObjName, classObj);\n");
      classHandler += s("  }\n"); // end of class scope
      content += classHandler;
    }


    //content += "\n}";





    // handlers
    QString handlers;


    // > classes
    for (const ClassDef &c : qAsConst(classDefs))
    {
      QString classHandler;
      const QString &className(c.mName);

      qWarning() << "handler for class" << className;


      // > statics
      for (const StaticFunction &sf : qAsConst(c.mStaticFunctions))
      {
        qWarning() << "handler for" << className << sf.mName;

        s _classHandler = readFile("handler1.tpl").arg(QString("%1_%2").arg(className, sf.mName)).arg(sf.mParams.count());
        //qWarning() << "?!?!?!?!" << _classHandler;
        classHandler += _classHandler;
        // TODO: !
        classHandler += "  return jerry_create_undefined();\n";
        classHandler += "}\n\n";
      }


      // > members
      for (const MemberFunction &m : qAsConst(c.mMemberFunctions))
      {
        classHandler += readFile("handler1.tpl").arg(QString("%1_%2").arg(className, m.mName)).arg(m.mParams.count());
        // TODO: !
        classHandler += "  return jerry_create_undefined();\n";
        classHandler += "}\n\n";
      }


      // > class creator
      if (!c.mMemberFunctions.isEmpty() || !c.mCtors.isEmpty()) // only for classes with other stuff than static functions
      {
        classHandler += s("jerry_value_t _rtjs_create_%1_object(%1 *class_ptr)\n").arg(className);
        classHandler += s("{\n");
        classHandler += s("  auto classObj = jerry_create_object();\n");
        classHandler += s("  jerry_set_object_native_pointer(classObj, (void *)class_ptr, nullptr);\n");

        for (const MemberFunction &m : qAsConst(c.mMemberFunctions))
        {
          classHandler += readFile("function.tpl").arg(m.mName).arg(QString("%1_%2").arg(className, m.mName)).arg("classObj");
        }

        classHandler += s("\n");
        classHandler += s("  return classObj;\n");
        classHandler += s("}\n\n");
      }


      // > ctors
      // TODO: don't create ctor if ctor deleted or private
      if (c.mCtors.isEmpty()) // create default ctor
      {
        classHandler += readFile("handler1.tpl").arg(QString("%1_ctor%2").arg(className).arg(0)).arg(0);

        //classHandler += s("jerry_value_t _rtjs_%1_ctor0_handler()\n").arg(className);
        //classHandler += s("{\n");
        classHandler += s("  auto *class_ptr = new %1;\n").arg(className);
        classHandler += s("  auto classObj = _rtjs_create_%1_object(class_ptr);\n").arg(className);
        classHandler += s("  jerry_set_object_native_pointer(classObj, (void *)class_ptr, nullptr);\n");

        for (const MemberFunction &m : qAsConst(c.mMemberFunctions))
        {
          classHandler += readFile("function.tpl").arg(m.mName).arg(QString("%1_%2").arg(className, m.mName)).arg("classObj");
        }

        classHandler += s("\n");
        classHandler += s("  return classObj;\n");
        classHandler += s("}\n\n");
      }


      // TODO: for (int ctorn = 0; ctorn < c.mCtors.count(); ctorn++)//const Ctor &ctor : qAsConst(c.mCtors))


      handlers += classHandler;
    }


    // > functions
    for(const Function &f : qAsConst(functions))
    {
      const QString &fnName(f.mName);
      qWarning() << "handler for function" << f.mName;

      handlers += readFile("handler1.tpl").arg(f.mName).arg(f.mParams.count());


      // get parameters
      QStringList pns;
      int pn = 0;
      for (const Parameter &p : qAsConst(f.mParams))
      {
        QString getter;

        if (p.paramType == ParamType::Boolean)
          getter = QString("  auto _param%1 = jerry_value_to_boolean(args[%1]);\n").arg(pn);
        else if (p.paramType == ParamType::Pointer)
        {
          getter += QString("  auto jsParam%1 = args[%1];\n").arg(pn);
          getter += QString("  void *param%1Ptr = nullptr;\n").arg(pn);
          getter += QString("  if (!jerry_get_object_native_pointer(jsParam%1, &param%1Ptr, nullptr))\n").arg(pn);
          getter += QString("    throw std::string(\"some handler called with no raw pointer behind it!\");\n");
          getter += QString("  auto *_param%1 = static_cast<%2>(param%1Ptr);\n").arg(pn).arg(p.mType);
        }
        else
          getter = "oopsgetter";

        handlers += getter;

        handlers += QString("  auto param%1 = _param%1;\n").arg(pn);

        pns += QString("param%1").arg(pn);
        pn++;
      }


      // call c/c++ function
      handlers += QString("  auto ret = %1(%2);\n").arg(fnName).arg(pns.join(", "));
      handlers += QString("  std::cerr << \"ret = \" << ret << std::endl;\n");

      switch (f.mReturnType)
      {
        case ParamType::Boolean:
        {
          handlers += QString("  return jerry_create_boolean(ret);\n");
          break;
        }

        case ParamType::Pointer:
        {
          // return new object with pointer in it

          handlers += "  jerry_value_t retObj = jerry_create_object();\n";
          handlers += "  jerry_set_object_native_pointer(retObj, (void *)ret, nullptr);\n";
          handlers += "  return retObj;\n";
          break;
        }

        default:
        {
          handlers += "  oopshandler\n";
          break;
        }
      }


      handlers += "\n}\n\n";
    }


    QString init(readFile("init-head.tpl"));


    for (const QString &include : qAsConst(includes))
      init += QString("#include \"%1\"\n").arg(include);
    for (const QString &include : qAsConst(sourceFiles))
      init += QString("#include \"%1\"\n").arg(include);


    init += "\n\n";

    init += handlers;
    init += readFile("init.tpl").arg("TestTarget").arg(content);


    QFile out(output);

    assert(out.open(QIODevice::WriteOnly));

    out.write(init.toUtf8());
    out.flush();

    //qWarning() << "init" << init;
  }


  qWarning() << "rtjs done";
  return 0;
}
