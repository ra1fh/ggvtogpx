/*
    ggvtogpx main module

    Copyright (C) 2022 Ralf Horstmann <ralf@ackstorm.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>

#include <memory>

#include "format.h"
#include "geodata.h"
#include "ggv_bin.h"
#include "ggv_ovl.h"
#include "ggv_xml.h"
#include "gpx.h"

static void process_files(const QString& formatName, const QString& infileName, const QString& outfileName, const QString& creator, bool testmode, int debug_level)
{
  if (debug_level > 2) {
    qDebug() << "process_files: format =" << formatName << " infile =" << infileName << " outfile =" << outfileName << " creator =" << creator;
  }

  Geodata geodata;
  geodata.setDebugLevel(debug_level);

  // Open the input file
  std::unique_ptr<QFile> infile;
  if (infileName == "-") {
    infile = std::make_unique<QFile>();
    if (!infile->open(stdin, QIODevice::ReadOnly)) {
      qCritical() << "error opening file" << infileName;
      exit(1);
    }
  } else {
    infile = std::make_unique<QFile>(infileName);
    if (!infile->open(QIODevice::ReadOnly)) {
      qCritical() << "error opening file" << infileName;
      exit(1);
    }
  }

  // Instantiate input formats
  std::list<std::unique_ptr<Format>> formats;
  formats.push_back(std::make_unique<GgvBinFormat>());
  formats.push_back(std::make_unique<GgvOvlFormat>());
  formats.push_back(std::make_unique<GgvXmlFormat>());

  // Determine which input format to use (either auto-probe or by
  // command line switch)
  Format* format = NULL;
  if (formatName == "") {
    for (auto&& f : std::as_const(formats)) {
      if (f->probe(infile.get())) {
        if (debug_level > 0) {
          qDebug().nospace() << "auto-probing " << f->getName() << ": true";
        }
        format = f.get();
        break;
      } else {
        if (debug_level > 0) {
          qDebug().nospace() << "auto-probing " << f->getName() << ": false";
        }
      }
    }
    if (!format) {
      qCritical() << "auto-probing failed";
      exit(1);
    }
  } else {
    for (auto&& f : std::as_const(formats)) {
      if (formatName == f->getName()) {
        format = f.get();
        break;
      }
    }
    if (!format) {
      qCritical() << "no such input format:" << formatName;
      exit(1);
    }
  }

  // Read the intput file
  format->setDebugLevel(debug_level);
  format->read(infile.get(), &geodata);

  // Tolerate empty output file to be able to run input code only with
  // debug enabled
  if (outfileName.isEmpty()) {
    return;
  }

  // Open the output file
  std::unique_ptr<QFile> outfile;
  if (outfileName == "-") {
    outfile = std::make_unique<QFile>();
    if (!outfile->open(stdout, QIODevice::WriteOnly | QIODevice::Text)) {
      qCritical() << "error: could not open stdout";
      exit(1);
    }
  } else {
    outfile = std::make_unique<QFile>(outfileName);
    if (!outfile->open(QIODevice::WriteOnly | QIODevice::Text)) {
      qCritical() << "error: could not open" << outfileName;
      exit(1);
    }
  }

  // Write GPX
  GpxFormat gpx;
  gpx.setCreator(creator);
  gpx.setTestmode(testmode);
  gpx.write(outfile.get(), &geodata);
  outfile.get()->close();
}

int main(int argc, char* argv[])
{
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName("ggvtogpx");
  QCoreApplication::setApplicationVersion("1.0");

  QCommandLineParser parser;
  parser.setApplicationDescription("\n"
                                   "Geogrid-Viewer OVL to GPX Converter. The input and output file\n"
                                   "options accept '-' for stdin or stdout. If no output file is\n"
                                   "given, the GPX output code will not run (useful for debugging).");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption debugLevelOption("D", "debug <level>", "debug");
  parser.addOption(debugLevelOption);

  QCommandLineOption inputTypeOption("i", "input <type> (ggv_bin, ggv_ovl)", "type");
  parser.addOption(inputTypeOption);

  QCommandLineOption inputFileOption("f", "input <file>", "file");
  parser.addOption(inputFileOption);

  QCommandLineOption outputTypeOption("o", "output <type> (ignored)", "type");
  parser.addOption(outputTypeOption);

  QCommandLineOption outputFileOption("F", "output <file>", "file");
  parser.addOption(outputFileOption);

  parser.addPositionalArgument("infile", "input file (alternative to -f)");
  parser.addPositionalArgument("outfile","output file (alternative to -F)");

  parser.process(app);

  int debug_level = 0;
  if (parser.isSet(debugLevelOption)) {
    bool ok = false;
    int num = parser.value(debugLevelOption).toInt(&ok);
    if (ok && num >= 0 && num <= 9) {
      debug_level = num;
    } else {
      qCritical() << qPrintable(app.applicationName()) << ": invalid debug level";
    }
  }

  QString infile("");
  QString outfile("");
  if (parser.positionalArguments().size() > 2) {
    qCritical() << qPrintable(app.applicationName()) << ": too many positional arguments";
    exit(1);
  } else if (parser.positionalArguments().size() == 2) {
    infile = parser.positionalArguments().at(0);
    outfile = parser.positionalArguments().at(1);
  } else if (parser.positionalArguments().size() == 1) {
    infile = parser.positionalArguments().at(0);
  }

  if (parser.isSet(inputFileOption)) {
    infile = parser.value(inputFileOption);
  }

  if (parser.isSet(outputFileOption)) {
    outfile = parser.value(outputFileOption);
  }

  QString creator = "ggvtogpx";
  if (qEnvironmentVariableIsSet("GGVTOGPX_CREATOR")) {
    creator = qEnvironmentVariable("GGVTOGPX_CREATOR");
  }

  bool testmode = false;
  if (qEnvironmentVariableIsSet("GGVTOGPX_TESTMODE")) {
    testmode = true;
  }

  QString formatName;
  if (parser.isSet(inputTypeOption)) {
    formatName = parser.value(inputTypeOption);
  }

  process_files(formatName, infile, outfile, creator, testmode, debug_level);
  exit(0);
}
