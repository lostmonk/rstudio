/*
 * SessionConsoleProcessInfoTests.cpp
 *
 * Copyright (C) 2009-17 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */
#include <session/SessionConsoleProcessInfo.hpp>

#define RSTUDIO_NO_TESTTHAT_ALIASES
#include <tests/TestThat.hpp>

namespace rstudio {
namespace session {
namespace console_process {
namespace tests {

using namespace console_process;

namespace {

const std::string emptyStr;
const std::string caption("Terminal 1");
const std::string title("/Users/roger/R");
const std::string handle1("unit-test01");
const std::string bogusHandle1("unit-test03");
const int sequence = 1;
const bool allowRestart = true;
const InteractionMode mode = InteractionAlways;
const size_t maxLines = 1000;

bool testHandle(const std::string& handle)
{
   return !handle.compare(handle1);
}

// helper returns true if ConsoleProcessInfo objs have same field values
bool sameCpi(const ConsoleProcessInfo& first, const ConsoleProcessInfo& second)
{
   return (!first.getCaption().compare(second.getCaption()) &&
           !first.getTitle().compare(second.getTitle()) &&
           !first.getHandle().compare(second.getHandle()) &&
           first.getTerminalSequence() == second.getTerminalSequence() &&
           first.getAllowRestart() == second.getAllowRestart() &&
           first.getInteractionMode() == second.getInteractionMode() &&
           first.getMaxOutputLines() == second.getMaxOutputLines() &&
           first.getShowOnOutput() == second.getShowOnOutput() &&
           first.getExitCode()  == second.getExitCode() &&
           first.getHasChildProcs() == second.getHasChildProcs());
}

} // anonymous namespace


TEST_CASE("ConsoleProcessInfo")
{
   SECTION("Create ConsoleProcessInfo and read properties")
   {
      ConsoleProcessInfo cpi(caption, title, handle1, sequence,
                             allowRestart, mode, maxLines);

      CHECK_FALSE(caption.compare(cpi.getCaption()));
      CHECK_FALSE(title.compare(cpi.getTitle()));
      CHECK_FALSE(handle1.compare(cpi.getHandle()));
      CHECK(cpi.getTerminalSequence() == sequence);
      CHECK(cpi.getAllowRestart() == allowRestart);
      CHECK(cpi.getInteractionMode() == mode);
      CHECK(cpi.getMaxOutputLines() == maxLines);

      CHECK_FALSE(cpi.getShowOnOutput());
      CHECK_FALSE(cpi.getExitCode());
      CHECK(cpi.getHasChildProcs());
   }

   SECTION("Generate a handle")
   {
      ConsoleProcessInfo cpi(emptyStr /*caption*/, emptyStr /*title*/,
                             emptyStr /*handle*/, kNoTerminal,
                             false /*allowRestart*/, InteractionNever,
                             0 /*maxLines*/);

      std::string handle = cpi.getHandle();
      CHECK(handle.empty());
      cpi.ensureHandle();
      handle = cpi.getHandle();
      CHECK_FALSE(handle.empty());
   }

   SECTION("Change properties")
   {
      ConsoleProcessInfo cpi(caption, title, handle1, sequence,
                             allowRestart, mode, maxLines);

      std::string altCaption("other caption");
      CHECK(altCaption.compare(caption));
      cpi.setCaption(altCaption);
      CHECK_FALSE(altCaption.compare(cpi.getCaption()));

      std::string altTitle("other title");
      CHECK(altTitle.compare(title));
      cpi.setTitle(altTitle);
      CHECK_FALSE(altTitle.compare(cpi.getTitle()));

      int altSequence = sequence + 1;
      cpi.setTerminalSequence(altSequence);
      CHECK(altSequence == cpi.getTerminalSequence());

      bool altAllowRestart = !allowRestart;
      cpi.setAllowRestart(altAllowRestart);
      CHECK(altAllowRestart == cpi.getAllowRestart());

      InteractionMode altMode = InteractionNever;
      CHECK_FALSE(altMode == mode);
      cpi.setInteractionMode(altMode);
      CHECK(altMode == cpi.getInteractionMode());

      size_t altMax = maxLines + 1;
      cpi.setMaxOutputLines(altMax);
      CHECK(altMax == cpi.getMaxOutputLines());

      bool altShowOnOutput = !cpi.getShowOnOutput();
      cpi.setShowOnOutput(altShowOnOutput);
      CHECK(altShowOnOutput == cpi.getShowOnOutput());

      bool altHasChildProcs = !cpi.getHasChildProcs();
      cpi.setHasChildProcs(altHasChildProcs);
      CHECK(altHasChildProcs == cpi.getHasChildProcs());
   }

   SECTION("Create ConsoleProcessInfo and read properties")
   {
      ConsoleProcessInfo cpi(caption, title, handle1, sequence,
                             allowRestart, mode, maxLines);

      const int exitCode = 14;
      cpi.setExitCode(exitCode);
      CHECK(cpi.getExitCode());
      CHECK(exitCode == *cpi.getExitCode());
   }

   SECTION("Save and load console proc metadata")
   {
      std::string orig = ConsoleProcessInfo::loadConsoleProcessMetadata();
      std::string newMetadata = "once upon a time";
      ConsoleProcessInfo::saveConsoleProcesses(newMetadata);
      std::string newSaved = ConsoleProcessInfo::loadConsoleProcessMetadata();
      CHECK_FALSE(newSaved.compare(newMetadata));
      ConsoleProcessInfo::saveConsoleProcesses(orig);
      newSaved = ConsoleProcessInfo::loadConsoleProcessMetadata();
      CHECK_FALSE(newSaved.compare(orig));
   }

   SECTION("Compare ConsoleProcInfos with different exit codes")
   {
      ConsoleProcessInfo cpiFirst(caption, title, handle1, sequence,
                                  allowRestart, mode, maxLines);
      ConsoleProcessInfo cpiSecond(caption, title, handle1, sequence,
                                   allowRestart, mode, maxLines);

      cpiFirst.setExitCode(1);
      cpiSecond.setExitCode(12);
      CHECK_FALSE(sameCpi(cpiFirst, cpiSecond));
   }

   SECTION("Persist and restore")
   {
      ConsoleProcessInfo cpiOrig(caption, title, handle1, sequence,
                                 allowRestart, mode, maxLines);

      core::json::Object origJson = cpiOrig.toJson();
      boost::shared_ptr<ConsoleProcessInfo> pCpiRestored =
            ConsoleProcessInfo::fromJson(origJson);
      CHECK(pCpiRestored);
      CHECK(sameCpi(cpiOrig, *pCpiRestored));
   }

   SECTION("Persist and restore for non-terminals")
   {
      // kNoTerminal triggers the older non-terminal behavior where
      // buffers are saved directly in this object and end up
      // persisted to the JSON. So to validate this we need to save some
      // output, persist to JSON, restore from JSON, and see that we get
      // back what we saved (trimmed based on maxLines). Be nice to abstract
      // all of this away, but bigger fish and all of that.
      ConsoleProcessInfo cpi(caption, title, handle1, kNoTerminal,
                             allowRestart, mode, maxLines);

      // bufferedOutput is the accessor for the non-terminal buffer cache
      std::string orig = cpi.bufferedOutput();
      CHECK(orig.empty());

      std::string strOrig("one\ntwo\nthree\n");

      // kNoTerminal mode buffer returns everything after the first \n
      // in the buffer
      cpi.appendToOutputBuffer('\n');
      cpi.appendToOutputBuffer(strOrig);
      std::string loaded = cpi.bufferedOutput();
      CHECK_FALSE(loaded.compare(strOrig));

      // add some more
      std::string orig2("four\nfive\nsix");
      cpi.appendToOutputBuffer(orig2);
      loaded = cpi.bufferedOutput();
      strOrig.append(orig2);
      CHECK_FALSE(loaded.compare(strOrig));
   }

   SECTION("Persist and restore for terminals")
   {
      // terminal sequence other than kNoTerminal triggers terminal
      // behavior where buffer are saved to an external file instead of
      // in the JSON. Same comment on the leaky abstractions here as for
      // previous non-terminal test.
      ConsoleProcessInfo cpi(caption, title, handle1, sequence,
                             allowRestart, mode, maxLines);

      // blow away anything that might have been left over from a previous
      // failed run
      cpi.deleteLogFile();

      // getSavedBuffer is the accessor for the terminal buffer cache
      std::string loaded = cpi.getSavedBuffer();
      CHECK(loaded.empty());

      std::string orig = "one\ntwo\nthree\nfour\nfive";
      cpi.appendToOutputBuffer(orig);
      loaded = cpi.getSavedBuffer();
      CHECK_FALSE(loaded.compare(orig));

      std::string orig2 = "\nsix\nseven\neight\nnine\nten\n";
      cpi.appendToOutputBuffer(orig2);
      loaded = cpi.getSavedBuffer();
      orig.append(orig2);
      CHECK_FALSE(loaded.compare(orig));

      cpi.deleteLogFile();
      loaded = cpi.getSavedBuffer();
      CHECK(loaded.empty());
   }

   SECTION("Delete unknown log files")
   {
      ConsoleProcessInfo cpiGood(caption, title, handle1, sequence,
                             allowRestart, mode, maxLines);
      ConsoleProcessInfo cpiBad(caption, title, bogusHandle1, sequence,
                             allowRestart, mode, maxLines);

      std::string orig1("hello how are you?\nthat is good\nhave a nice day");
      std::string bogus1("doom");

      cpiGood.appendToOutputBuffer(orig1);
      cpiBad.appendToOutputBuffer(bogus1);

      std::string loadedGood = cpiGood.getSavedBuffer();
      CHECK_FALSE(loadedGood.compare(orig1));
      std::string loadedBad = cpiBad.getSavedBuffer();
      CHECK_FALSE(loadedBad.compare(bogus1));

      cpiGood.deleteOrphanedLogs(testHandle);
      cpiBad.deleteOrphanedLogs(testHandle);

      loadedGood = cpiGood.getSavedBuffer();
      CHECK_FALSE(loadedGood.compare(orig1));
      loadedBad = cpiBad.getSavedBuffer();
      CHECK(loadedBad.empty());

      cpiGood.deleteLogFile();
      cpiBad.deleteLogFile();
   }
}

} // end namespace tests
} // end namespace console_process
} // end namespace session
} // end namespace rstudio
