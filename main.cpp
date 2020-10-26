#include <QCoreApplication>
#include <QFile>
#include <QString>
#include <QDebug>

#include <iostream>

#include <cppast/code_generator.hpp>         // for generate_code()
#include <cppast/cpp_entity_kind.hpp>        // for the cpp_entity_kind definition
#include <cppast/cpp_forward_declarable.hpp> // for is_definition()
#include <cppast/cpp_namespace.hpp>          // for cpp_namespace
#include <cppast/cpp_member_function.hpp>
#include <cppast/libclang_parser.hpp> // for libclang_parser, libclang_compile_config, cpp_entity,...
#include <cppast/visitor.hpp>         // for visit()




class Parameter
{
public:
  QString mName;
  QString mType;
};


class Function
{
public:
  QString mName;
  QVector<Parameter> mParams;
};


class MemberFunction : public Function
{
public:

};


class StaticFunction : public Function
{
public:

};


class ClassDef
{
public:
  bool mValid = false;
  QString mName;
  QVector<MemberFunction> mMemberFunctions;
  QVector<StaticFunction> mStaticFunctions;
};


void getFunctionParameters(Function &function, cppast::detail::iteratable_intrusive_list<cppast::cpp_function_parameter> params)
{
  std::for_each(params.begin(), params.end(), [&function](const cppast::cpp_function_parameter &param)
  {
    QString paramName(QString::fromStdString(param.name()));
    qWarning() << "param type int" << (int)param.type().kind() << "for param with name" << paramName;

    QString type;
    if (param.type().kind() == cppast::cpp_type_kind::builtin_t)
    {
      auto& builtin = static_cast<const cppast::cpp_builtin_type &>(param.type());

      switch (builtin.builtin_type_kind())
      {
        case cppast::cpp_builtin_type_kind::cpp_bool:
        {
          type = "bool";
          break;
        }

        default:
        {
          type = "auto"; // oops, try to hide the failure :)
          break;
        }
      }
    }


    function.mParams += { paramName, type };
    qWarning() << "parameter" << paramName << "is of type" << type;
  });
}




int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);

  cppast::libclang_compile_config config;

  const QStringList args(app.arguments());

  if (args.count() < 2)
  {
    qDebug() << "usage:" << args.at(0) << "<cpp file> [include paths ...]";
    return -1;
  }



  enum class ArgType
  {
    Skip,
    Source,
    Include,
    Definition
  };

  QStringList sourceFiles;
  static QStringList paramSwitches({ "-I", "-D" });
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
    }

    switch (argType)
    {
      case ArgType::Skip:
      {
        qWarning() << "(skipped)";
        argType = ArgType::Source;
        continue;
      }

      case ArgType::Source:
      {
        qWarning() << "(added as source)";
        sourceFiles += arg;
        break;
      }

      case ArgType::Include:
      {
        QStringList includes(arg.split(";"));
        for (const QString &include : qAsConst(includes))
        {
          qWarning() << "(added as include) [" << include << "]";
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

          qWarning() << "(added as definition) [" << x << "]";
        }

        break;
      }
    }
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



  if (sourceFiles.isEmpty())
  {
    qWarning() << "no source files given";
    return -1;
  }



  for (const QString &filename : qAsConst(sourceFiles))
  {
    qWarning() << "parsing file" << filename;

    // parse the file
    auto file = parser.parse(idx, filename.toStdString(), config);
    if (parser.error())
    {
      qDebug() << "parser error";
      return -1;
    }


    ClassDef currentClass;


    cppast::visit(static_cast<const cppast::cpp_file &>(*file), [&](const cppast::cpp_entity& e, cppast::visitor_info info)
    {
      qWarning() << "visiting entity" << QString::fromStdString(e.name()) << "kind =" << (int)e.kind();



      if (e.parent().has_value())
        qWarning() << "parent" << QString::fromStdString(e.parent().value().name());

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

        currentClass.mStaticFunctions += staticFunction;

        //currentClass.mStaticFunctions += { QString::fromStdString(e.name()), {} };
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



  }



  qWarning() << "rtjs done";
  return 0;
}
