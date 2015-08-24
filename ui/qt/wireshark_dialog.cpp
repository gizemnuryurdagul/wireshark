/* wireshark_dialog.cpp
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib.h>

#include "cfile.h"

#include <epan/packet.h>
#include <epan/tap.h>

#include "wireshark_dialog.h"

#include <QMessageBox>

#include "wireshark_application.h"

// To do:
// - Use a dynamic property + Q_PROPERTY for the subtitle.
// - Save and load recent geometry.

WiresharkDialog::WiresharkDialog(QWidget &, CaptureFile &capture_file) :
    QDialog(NULL, Qt::Window),
    cap_file_(capture_file),
    file_closed_(false),
    retap_depth_(0),
    dialog_closed_(false)
{
    setWindowIcon(wsApp->normalIcon());
    setWindowTitleFromSubtitle();

    connect(&cap_file_, SIGNAL(captureFileRetapStarted()), this, SLOT(beginRetapPackets()));
    connect(&cap_file_, SIGNAL(captureFileRetapFinished()), this, SLOT(endsRetapPackets()));
    connect(&cap_file_, SIGNAL(captureFileClosing()), this, SLOT(captureFileClosing()));
    connect(&cap_file_, SIGNAL(captureFileClosed()), this, SLOT(captureFileClosing()));
}

void WiresharkDialog::accept()
{
    QDialog::accept();

    // Cancel any taps in progress?
    // cap_file_.setCaptureStopFlag();
    removeTapListeners();
    dialog_closed_ = true;
    tryDeleteLater();
}

// XXX Should we do this in WiresharkDialog?
void WiresharkDialog::reject()
{
    QDialog::reject();

    // Cancel any taps in progress?
    // cap_file_.setCaptureStopFlag();
    removeTapListeners();
    dialog_closed_ = true;
    tryDeleteLater();
}


void WiresharkDialog::setWindowSubtitle(const QString &subtitle)
{
    subtitle_ = subtitle;
    setWindowTitleFromSubtitle();
}

void WiresharkDialog::setWindowTitleFromSubtitle()
{
    QString title = wsApp->windowTitleString(QStringList() << subtitle_ << cap_file_.fileTitle());
    QDialog::setWindowTitle(title);
}

// See if we can destroy ourselves. The user may have clicked "Close" while
// we were deep in the bowels of a routine that retaps packets. Track our
// tapping state using retap_depth_ and our closed state using dialog_closed_.
void WiresharkDialog::tryDeleteLater()
{
    // In many of our subclasses, if the user clicks "Apply" followed by
    // "Close" we can end up calling fillTree after calling tryDeleteLater.
    // Disconnecting our slots here prevents that (in Qt5 at least).
    disconnect();
    if (retap_depth_ < 1 && dialog_closed_) deleteLater();
}

void WiresharkDialog::updateWidgets()
{
    setWindowTitleFromSubtitle();
}

bool WiresharkDialog::registerTapListener(const char *tap_name, void *tap_data, const char *filter, guint flags, void(*tap_reset)(void *), gboolean(*tap_packet)(void *, struct _packet_info *, struct epan_dissect *, const void *), void(*tap_draw)(void *))
{
    GString *error_string = register_tap_listener(tap_name, tap_data, filter, flags,
                                                  tap_reset, tap_packet, tap_draw);
    if (error_string) {
        QMessageBox::warning(this, tr("Failed to attach to tap \"%1\"").arg(tap_name),
                             error_string->str);
        g_string_free(error_string, TRUE);
        return false;
    }

    tap_listeners_ << tap_data;
    return true;
}

void WiresharkDialog::endRetapPackets()
{
    retap_depth_--;
    tryDeleteLater();
}

void WiresharkDialog::removeTapListeners()
{
    while (!tap_listeners_.isEmpty()) {
        remove_tap_listener(tap_listeners_.takeFirst());
    }
}

void WiresharkDialog::captureFileClosing()
{
    removeTapListeners();
    file_closed_ = true;
    setWindowTitleFromSubtitle();
    updateWidgets();
}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
