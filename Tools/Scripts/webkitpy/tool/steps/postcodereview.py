# Copyright (C) 2010 Google Inc. All rights reserved.
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

from webkitpy.tool.steps.abstractstep import AbstractStep
from webkitpy.tool.steps.options import Options


class PostCodeReview(AbstractStep):
    @classmethod
    def options(cls):
        return AbstractStep.options() + [
            Options.cc,
            Options.description,
        ]

    def run(self, state):
        patch = state.get("patch")
        bug_id = patch.bug_id()
        title = patch.name()

        # If the issue already exists, then the message becomes the label
        # of the new patch. Otherwise, it becomes the title of the whole
        # issue.
        if title:
            # This is the common case for the the first "upload" command.
            message = title
        elif bug_id:
            # This is the common case for the "post" command and
            # subsequent runs of the "upload" command.
            message = "Code review for %s" % self._tool.bugs.bug_url_for_bug_id(bug_id)
        else:
            # Unreachable with our current commands, but we might hit
            # this case if we support bug-less code reviews.
            message = "Code review"

        # Use the bug ID as the rietveld issue number. This means rietveld code reviews
        # when there are multiple different patches on a bug will be a bit wonky, but
        # webkit-patch assumes one-patch-per-bug.
        created_issue = self._tool.codereview.post(diff=self.cached_lookup(state, "diff"),
                                                   message=message,
                                                   codereview_issue=bug_id,
                                                   cc=self._options.cc,
                                                   patch_id=patch.id())

        self._tool.bugs.set_flag_on_attachment(patch.id(), 'in-rietveld', '+')
