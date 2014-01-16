# Copyright (c) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import itertools
import random
import re

from webkitpy.common.config import irc as config_irc
from webkitpy.common.config import urls
from webkitpy.common.config.committers import CommitterList
from webkitpy.common.net.web import Web
from webkitpy.common.system.executive import ScriptError
from webkitpy.tool.bot.queueengine import TerminateQueue
from webkitpy.tool.grammar import join_with_separators


def _post_error_and_check_for_bug_url(tool, nicks_string, exception):
    tool.irc().post("%s" % exception)
    bug_id = urls.parse_bug_id(exception.output)
    if bug_id:
        bug_url = tool.bugs.bug_url_for_bug_id(bug_id)
        tool.irc().post("%s: Ugg...  Might have created %s" % (nicks_string, bug_url))


# FIXME: Merge with Command?
class IRCCommand(object):
    usage_string = None
    help_string = None

    def execute(self, nick, args, tool, sheriff):
        raise NotImplementedError("subclasses must implement")

    @classmethod
    def usage(cls, nick):
        return "%s: Usage: %s" % (nick, cls.usage_string)

    @classmethod
    def help(cls, nick):
        return "%s: %s" % (nick, cls.help_string)


class CreateBug(IRCCommand):
    usage_string = "create-bug BUG_TITLE"
    help_string = "Creates a Bugzilla bug with the given title."

    def execute(self, nick, args, tool, sheriff):
        if not args:
            return self.usage(nick)

        bug_title = " ".join(args)
        bug_description = "%s\nRequested by %s on %s." % (bug_title, nick, config_irc.channel)

        # There happens to be a committers list hung off of Bugzilla, so
        # re-using that one makes things easiest for now.
        requester = tool.bugs.committers.contributor_by_irc_nickname(nick)
        requester_email = requester.bugzilla_email() if requester else None

        try:
            bug_id = tool.bugs.create_bug(bug_title, bug_description, cc=requester_email, assignee=requester_email)
            bug_url = tool.bugs.bug_url_for_bug_id(bug_id)
            return "%s: Created bug: %s" % (nick, bug_url)
        except Exception, e:
            return "%s: Failed to create bug:\n%s" % (nick, e)


class Help(IRCCommand):
    usage_string = "help [COMMAND]"
    help_string = "Provides help on individual sheriffbot commands."

    def execute(self, nick, args, tool, sheriff):
        if args:
            for command_name in args:
                if command_name in commands:
                    self._post_command_help(nick, tool, commands[command_name])
        else:
            tool.irc().post("%s: Available commands: %s" % (nick, ", ".join(sorted(visible_commands.keys()))))
            tool.irc().post('%s: Type "sheriffbot: help COMMAND" for help on individual commands.' % nick)

    def _post_command_help(self, nick, tool, command):
        tool.irc().post(command.usage(nick))
        tool.irc().post(command.help(nick))
        aliases = " ".join(sorted(filter(lambda alias: commands[alias] == command and alias not in visible_commands, commands)))
        if aliases:
            tool.irc().post("%s: Aliases: %s" % (nick, aliases))


class Hi(IRCCommand):
    usage_string = "hi"
    help_string = "Retrieves a random quip from Bugzilla."

    def execute(self, nick, args, tool, sheriff):
        quips = tool.bugs.quips()
        quips.append('"Only you can prevent forest fires." -- Smokey the Bear')
        return random.choice(quips)


class Restart(IRCCommand):
    usage_string = "restart"
    help_string = "Restarts sherrifbot.  Will update its WebKit checkout, and re-join the channel momentarily."

    def execute(self, nick, args, tool, sheriff):
        tool.irc().post("Restarting...")
        raise TerminateQueue()


class RollChromiumDEPS(IRCCommand):
    usage_string = "roll-chromium-deps REVISION"
    help_string = "Rolls WebKit's Chromium DEPS to the given revision. Give LKGR as the revision number to use the last known good revision. This lands an unreviewed patch and provides the bug URL."

    def _parse_args(self, args):
        if not args:
            return None
        revision = args[0].lstrip("r")
        if revision == "LKGR" or revision.isdigit():
            return revision
        return None

    def _expand_irc_nickname(self, nick):
        contributor = CommitterList().contributor_by_irc_nickname(nick)
        if contributor:
            return str(contributor)
        return nick

    def execute(self, nick, args, tool, sheriff):
        revision = self._parse_args(args)
        if not revision:
            return self.usage(nick)

        roll_target = "r%s" % revision if revision.isdigit() else "last-known good revision"
        tool.irc().post("%s: Rolling Chromium DEPS to %s" % (nick, roll_target))
        changelog_message = "Unreviewed.  Rolled Chromium DEPS to %s.  Requested by %s via sheriffbot.\n\n" % (roll_target, self._expand_irc_nickname(nick))

        try:
            bug_id = sheriff.post_chromium_deps_roll(revision, roll_target, changelog_message)
            bug_url = tool.bugs.bug_url_for_bug_id(bug_id)
            return "%s: Created DEPS roll: %s" % (nick, bug_url)
        except ScriptError, e:
            tool.irc().post("%s: Failed to create DEPS roll:" % nick)
            pre_bug_error_messages = [
                r"Current Chromium DEPS revision \d+ is newer than \d+\.",
                r"Unable to update Chromium DEPS\.",
                r"Unable to parse LKGR from: .*",
                r"Unable to reach LKGR source: .*",
                r"Invalid revision number\.",
            ]
            for message in pre_bug_error_messages:
                match = re.search(message, e.output)
                if match:
                    return "%s: %s" % (nick, match.group(0))
            _post_error_and_check_for_bug_url(tool, nick, e)


class Rollout(IRCCommand):
    usage_string = "rollout SVN_REVISION [SVN_REVISIONS] REASON"
    help_string = "Opens a rollout bug, CCing author + reviewer, and attaching the reverse-diff of the given revisions marked as commit-queue=?."

    def _extract_revisions(self, arg):
        revision_list = []
        possible_revisions = arg.split(",")
        for revision in possible_revisions:
            revision = revision.strip()
            if not revision:
                continue
            revision = revision.lstrip("r")
            # If one part of the arg isn't in the correct format,
            # then none of the arg should be considered a revision.
            if not revision.isdigit():
                return None
            revision_list.append(int(revision))
        return revision_list

    def _parse_args(self, args):
        if not args:
            return (None, None)

        svn_revision_list = []
        remaining_args = args[:]
        # First process all revisions.
        while remaining_args:
            new_revisions = self._extract_revisions(remaining_args[0])
            if not new_revisions:
                break
            svn_revision_list += new_revisions
            remaining_args = remaining_args[1:]

        # Was there a revision number?
        if not len(svn_revision_list):
            return (None, None)

        # Everything left is the reason.
        rollout_reason = " ".join(remaining_args)
        return svn_revision_list, rollout_reason

    def _responsible_nicknames_from_revisions(self, tool, sheriff, svn_revision_list):
        commit_infos = map(tool.checkout().commit_info_for_revision, svn_revision_list)
        nickname_lists = map(sheriff.responsible_nicknames_from_commit_info, commit_infos)
        return sorted(set(itertools.chain(*nickname_lists)))

    def _nicks_string(self, tool, sheriff, requester_nick, svn_revision_list):
        # FIXME: _parse_args guarentees that our svn_revision_list is all numbers.
        # However, it's possible our checkout will not include one of the revisions,
        # so we may need to catch exceptions from commit_info_for_revision here.
        target_nicks = [requester_nick] + self._responsible_nicknames_from_revisions(tool, sheriff, svn_revision_list)
        return ", ".join(target_nicks)

    def _update_working_copy(self, tool):
        tool.scm().discard_local_changes()
        tool.executive.run_and_throw_if_fail(tool.deprecated_port().update_webkit_command(), quiet=True, cwd=tool.scm().checkout_root)

    def execute(self, nick, args, tool, sheriff):
        svn_revision_list, rollout_reason = self._parse_args(args)

        if (not svn_revision_list or not rollout_reason):
            return self.usage(nick)

        revision_urls_string = join_with_separators([urls.view_revision_url(revision) for revision in svn_revision_list])
        tool.irc().post("%s: Preparing rollout for %s ..." % (nick, revision_urls_string))

        self._update_working_copy(tool)

        # FIXME: IRCCommand should bind to a tool and have a self._tool like Command objects do.
        # Likewise we should probably have a self._sheriff.
        nicks_string = self._nicks_string(tool, sheriff, nick, svn_revision_list)

        try:
            complete_reason = "%s (Requested by %s on %s)." % (
                rollout_reason, nick, config_irc.channel)
            bug_id = sheriff.post_rollout_patch(svn_revision_list, complete_reason)
            bug_url = tool.bugs.bug_url_for_bug_id(bug_id)
            tool.irc().post("%s: Created rollout: %s" % (nicks_string, bug_url))
        except ScriptError, e:
            tool.irc().post("%s: Failed to create rollout patch:" % nicks_string)
            _post_error_and_check_for_bug_url(tool, nicks_string, e)


class Sheriffs(IRCCommand):
    usage_string = "sheriffs"
    help_string = "Retrieves who the current Chromium WebKit sheriffs are from: %s" % urls.chromium_webkit_sheriff_url

    def _retrieve_webkit_sheriffs(self, tool, url):
        try:
            sheriff_js = tool.web.get_binary(url, True)
        except:
            return None
        if sheriff_js == None:
            return None

        match = re.search(r"document.write\('(.*)'\)", sheriff_js)

        try:
            return match.group(1)
        except:
            return None

    def execute(self, nick, args, tool, sheriff):
        if not args:
            url = urls.chromium_webkit_sheriff_url
        else:
            url = args[0]

        sheriffs = self._retrieve_webkit_sheriffs(tool, url)
        if sheriffs == None:
            return "%s: Failed to parse URL: %s" % (nick, url)

        sheriff_name = "Chromium Webkit sheriff"
        sheriff_count = len(sheriffs.split())
        if sheriff_count == 0:
            return "%s: There are no %ss currently assigned." % (nick, sheriff_name)
        if sheriff_count == 1:
            return "%s: The current %s is: %s" % (nick, sheriff_name, sheriffs)
        return "%s: The current %ss are: %s" % (nick, sheriff_name, sheriffs)


class Whois(IRCCommand):
    usage_string = "whois SEARCH_STRING"
    help_string = "Searches known contributors and returns any matches with irc, email and full name. Wild card * permitted."

    def _nick_or_full_record(self, contributor):
        if contributor.irc_nicknames:
            return ', '.join(contributor.irc_nicknames)
        return unicode(contributor)

    def execute(self, nick, args, tool, sheriff):
        if not args:
            return self.usage(nick)
        search_string = " ".join(args)
        # FIXME: We should get the ContributorList off the tool somewhere.
        contributors = CommitterList().contributors_by_search_string(search_string)
        if not contributors:
            return "%s: Sorry, I don't know any contributors matching '%s'." % (nick, search_string)
        if len(contributors) > 5:
            return "%s: More than 5 contributors match '%s', could you be more specific?" % (nick, search_string)
        if len(contributors) == 1:
            contributor = contributors[0]
            if not contributor.irc_nicknames:
                return "%s: %s hasn't told me their nick. Boo hoo :-(" % (nick, contributor)
            if contributor.emails and search_string.lower() not in map(lambda email: email.lower(), contributor.emails):
                formattedEmails = ', '.join(contributor.emails)
                return "%s: %s is %s (%s). Why do you ask?" % (nick, search_string, self._nick_or_full_record(contributor), formattedEmails)
            else:
                return "%s: %s is %s. Why do you ask?" % (nick, search_string, self._nick_or_full_record(contributor))
        contributor_nicks = map(self._nick_or_full_record, contributors)
        contributors_string = join_with_separators(contributor_nicks, only_two_separator=" or ", last_separator=', or ')
        return "%s: I'm not sure who you mean?  %s could be '%s'." % (nick, contributors_string, search_string)


# FIXME: Lame.  We should have an auto-registering CommandCenter.
visible_commands = {
    "create-bug": CreateBug,
    "help": Help,
    "hi": Hi,
    "restart": Restart,
    "roll-chromium-deps": RollChromiumDEPS,
    "rollout": Rollout,
    "sheriffs": Sheriffs,
    "whois": Whois,
}

# Add revert as an "easter egg" command. Why?
# revert is the same as rollout and it would be confusing to list both when
# they do the same thing. However, this command is a very natural thing for
# people to use and it seems silly to have them hunt around for "rollout" instead.
commands = visible_commands.copy()
commands["revert"] = Rollout
commands["gardeners"] = Sheriffs
# Enough people misspell "sheriffs" that they've requested aliases for the command.
commands["sherriffs"] = Sheriffs
commands["sherifs"] = Sheriffs
commands["sherrifs"] = Sheriffs
# "hello" Alias for "hi" command for the purposes of testing aliases
commands["hello"] = Hi
