/* Copyright 2011-2021 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <list>

#include <rfb/encodings.h>

#if defined(HAVE_GNUTLS) || defined(HAVE_NETTLE)
#include <rfb/Security.h>
#include <rfb/SecurityClient.h>
#ifdef HAVE_GNUTLS
#include <rfb/CSecurityTLS.h>
#endif
#endif

#include "OptionsDialog.h"
#include "i18n.h"
#include "menukey.h"
#include "parameters.h"

#include "fltk/layout.h"
#include "fltk/util.h"
#include "fltk/Fl_Monitor_Arrangement.h"
#include "fltk/Fl_Navigation.h"

#ifdef __APPLE__
#include "cocoa.h"
#endif

#include <FL/Fl.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>

std::map<OptionsCallback*, void*> OptionsDialog::callbacks;

static std::set<OptionsDialog *> instances;

OptionsDialog::OptionsDialog()
  // TRANSLATORS: Title of the viewer options dialog window
  : Fl_Window(580, 420, _("YVNC options"))
{
  int x, y;
  Fl_Navigation *navigation;
  Fl_Button *button;

  // Odd dimensions to get rid of extra borders
  // FIXME: We need to retain the top border on Windows as they don't
  //        have any separator for the caption, which looks odd
#ifdef WIN32
  navigation = new Fl_Navigation(-1, 0, w()+2,
                                 h() - OUTER_MARGIN - BUTTON_HEIGHT - OUTER_MARGIN);
#else
  navigation = new Fl_Navigation(-1, -1, w()+2,
                                 h()+1 - OUTER_MARGIN - BUTTON_HEIGHT - OUTER_MARGIN);
#endif
  {
    int tx, ty, tw, th;

    navigation->client_area(tx, ty, tw, th, 150);

    createCompressionPage(tx, ty, tw, th);
    createSecurityPage(tx, ty, tw, th);
    createInputPage(tx, ty, tw, th);
    createDisplayPage(tx, ty, tw, th);
    createMiscPage(tx, ty, tw, th);
  }

  navigation->end();

  x = w() - BUTTON_WIDTH * 2 - INNER_MARGIN - OUTER_MARGIN;
  y = h() - BUTTON_HEIGHT - OUTER_MARGIN;

  // TRANSLATORS: Button that dismisses the dialog without saving
  button = new Fl_Button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("Cancel"));
  button->callback(this->handleCancel, this);

  x += BUTTON_WIDTH + INNER_MARGIN;

  // TRANSLATORS: Button that confirms the dialog and applies changes
  button = new Fl_Return_Button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("OK"));
  button->callback(this->handleOK, this);

  callback(this->handleCancel, this);

  set_modal();

  if (instances.size() == 0)
    Fl::add_handler(fltk_event_handler);
  instances.insert(this);
}


OptionsDialog::~OptionsDialog()
{
  instances.erase(this);

  if (instances.size() == 0)
    Fl::remove_handler(fltk_event_handler);
}


void OptionsDialog::showDialog(void)
{
  static OptionsDialog *dialog = nullptr;

  if (!dialog)
    dialog = new OptionsDialog();

  if (dialog->shown())
    return;

  dialog->show();
}


void OptionsDialog::addCallback(OptionsCallback *cb, void *data)
{
  callbacks[cb] = data;
}


void OptionsDialog::removeCallback(OptionsCallback *cb)
{
  callbacks.erase(cb);
}


void OptionsDialog::show(void)
{
  /* show() gets called for raise events as well */
  if (!shown())
    loadOptions();

  Fl_Window::show();
}


void OptionsDialog::loadOptions(void)
{
  /* Compression */
  autoselectCheckbox->value(autoSelect);

  if (preferredEncoding == "Tight")
    tightButton->setonly();
  else if (preferredEncoding == "ZRLE")
    zrleButton->setonly();
  else if (preferredEncoding == "Hextile")
    hextileButton->setonly();
#ifdef HAVE_H264
  else if (preferredEncoding == "H.264")
    h264Button->setonly();
#endif
#ifdef HAVE_H265
  else if (preferredEncoding == "H.265")
    h265Button->setonly();
#endif
  else if (preferredEncoding == "Raw")
    rawButton->setonly();

  if (fullColour)
    fullcolorCheckbox->setonly();
  else {
    switch (lowColourLevel) {
    case 0:
      verylowcolorCheckbox->setonly();
      break;
    case 1:
      lowcolorCheckbox->setonly();
      break;
    case 2:
      mediumcolorCheckbox->setonly();
      break;
    }
  }

  char digit[2] = "0";

  compressionCheckbox->value(customCompressLevel);
  jpegCheckbox->value(!noJpeg);
  digit[0] = '0' + compressLevel;
  compressionInput->value(digit);
  digit[0] = '0' + qualityLevel;
  jpegInput->value(digit);

  handleAutoselect(autoselectCheckbox, this);
  handleCompression(compressionCheckbox, this);
  handleJpeg(jpegCheckbox, this);

#if defined(HAVE_GNUTLS) || defined(HAVE_NETTLE)
  /* Security */
  rfb::Security security(rfb::SecurityClient::secTypes);

  std::list<uint8_t> secTypes;

  std::list<uint32_t> secTypesExt;

  encNoneCheckbox->value(false);
#ifdef HAVE_GNUTLS
  encTLSCheckbox->value(false);
  encX509Checkbox->value(false);
#endif
#ifdef HAVE_NETTLE
  encRSAAESCheckbox->value(false);
#endif

  authNoneCheckbox->value(false);
  authVncCheckbox->value(false);
  authPlainCheckbox->value(false);

  secTypes = security.GetEnabledSecTypes();
  for (uint8_t type : secTypes) {
    switch (type) {
    case rfb::secTypeNone:
      encNoneCheckbox->value(true);
      authNoneCheckbox->value(true);
      break;
    case rfb::secTypeVncAuth:
      encNoneCheckbox->value(true);
      authVncCheckbox->value(true);
      break;
    }
  }

  secTypesExt = security.GetEnabledExtSecTypes();
  for (uint32_t type : secTypesExt) {
    switch (type) {
    case rfb::secTypePlain:
      encNoneCheckbox->value(true);
      authPlainCheckbox->value(true);
      break;
#ifdef HAVE_GNUTLS
    case rfb::secTypeTLSNone:
      encTLSCheckbox->value(true);
      authNoneCheckbox->value(true);
      break;
    case rfb::secTypeTLSVnc:
      encTLSCheckbox->value(true);
      authVncCheckbox->value(true);
      break;
    case rfb::secTypeTLSPlain:
      encTLSCheckbox->value(true);
      authPlainCheckbox->value(true);
      break;
    case rfb::secTypeX509None:
      encX509Checkbox->value(true);
      authNoneCheckbox->value(true);
      break;
    case rfb::secTypeX509Vnc:
      encX509Checkbox->value(true);
      authVncCheckbox->value(true);
      break;
    case rfb::secTypeX509Plain:
      encX509Checkbox->value(true);
      authPlainCheckbox->value(true);
      break;
#endif
#ifdef HAVE_NETTLE
    case rfb::secTypeRA2:
    case rfb::secTypeRA256:
      encRSAAESCheckbox->value(true);
      authVncCheckbox->value(true);
      authPlainCheckbox->value(true);
      break;
    case rfb::secTypeRA2ne:
    case rfb::secTypeRAne256:
      authVncCheckbox->value(true);
      /* fall through */
    case rfb::secTypeDH:
    case rfb::secTypeMSLogonII:
      encNoneCheckbox->value(true);
      authPlainCheckbox->value(true);
      break;
#endif
    
    }
  }

#ifdef HAVE_GNUTLS
  caInput->value(rfb::CSecurityTLS::X509CA);
  crlInput->value(rfb::CSecurityTLS::X509CRL);

  handleX509(encX509Checkbox, this);
#endif
#endif

  /* Input */
  viewOnlyCheckbox->value(viewOnly);
  emulateMBCheckbox->value(emulateMiddleButton);
  acceptClipboardCheckbox->value(acceptClipboard);
#if !defined(WIN32) && !defined(__APPLE__)
  setPrimaryCheckbox->value(setPrimary);
#endif
  sendClipboardCheckbox->value(sendClipboard);
#if !defined(WIN32) && !defined(__APPLE__)
  sendPrimaryCheckbox->value(sendPrimary);
#endif
  systemKeysCheckbox->value(fullscreenSystemKeys);

  menuKeyChoice->value(0);

  for (int idx = 0; idx < getMenuKeySymbolCount(); idx++)
    if (menuKey == getMenuKeySymbols()[idx].name)
      menuKeyChoice->value(idx + 1);

  /* Display */
  if (!fullScreen) {
    windowedButton->setonly();
  } else {
    if (fullScreenMode == "all") {
      allMonitorsButton->setonly();
    } else if (fullScreenMode == "selected") {
      selectedMonitorsButton->setonly();
    } else {
      currentMonitorButton->setonly();
    }
  }

  monitorArrangement->value(fullScreenSelectedMonitors.getMonitors());

  handleFullScreenMode(selectedMonitorsButton, this);

  if (strcmp(desktopSize, "") != 0) {
    int w, h;
    if (sscanf(desktopSize, "%dx%d", &w, &h) == 2) {
      char buf[16];
      snprintf(buf, sizeof(buf), "%d", w);
      desktopWidthInput->value(buf);
      snprintf(buf, sizeof(buf), "%d", h);
      desktopHeightInput->value(buf);
      desktopSizeCheckbox->value(true);
    } else {
      desktopSizeCheckbox->value(false);
    }
  } else {
    desktopSizeCheckbox->value(false);
  }

  remoteResizeCheckbox->value(::remoteResize);

  handleDesktopSize(desktopSizeCheckbox, this);

  /* Misc. */
  sharedCheckbox->value(shared);
  reconnectCheckbox->value(reconnectOnError);
  recordInput->value(recordFile.getValueStr().c_str());
  alwaysCursorCheckbox->value(alwaysCursor);
  if (cursorType == "system") {
    cursorTypeChoice->value(1);
  } else {
    // Default
    cursorTypeChoice->value(0);
  }
  handleAlwaysCursor(alwaysCursorCheckbox, this);
}


void OptionsDialog::storeOptions(void)
{
  /* Compression */
  autoSelect.setParam(autoselectCheckbox->value());

  if (tightButton->value())
    preferredEncoding.setParam(rfb::encodingName(rfb::encodingTight));
  else if (zrleButton->value())
    preferredEncoding.setParam(rfb::encodingName(rfb::encodingZRLE));
  else if (hextileButton->value())
    preferredEncoding.setParam(rfb::encodingName(rfb::encodingHextile));
#ifdef HAVE_H264
  else if (h264Button->value())
    preferredEncoding.setParam(rfb::encodingName(rfb::encodingH264));
#endif
#ifdef HAVE_H265
  else if (h265Button->value())
    preferredEncoding.setParam(rfb::encodingName(rfb::encodingH265));
#endif
  else if (rawButton->value())
    preferredEncoding.setParam(rfb::encodingName(rfb::encodingRaw));

  fullColour.setParam(fullcolorCheckbox->value());
  if (verylowcolorCheckbox->value())
    lowColourLevel.setParam(0);
  else if (lowcolorCheckbox->value())
    lowColourLevel.setParam(1);
  else if (mediumcolorCheckbox->value())
    lowColourLevel.setParam(2);

  customCompressLevel.setParam(compressionCheckbox->value());
  noJpeg.setParam(!jpegCheckbox->value());
  compressLevel.setParam(atoi(compressionInput->value()));
  qualityLevel.setParam(atoi(jpegInput->value()));

#if defined(HAVE_GNUTLS) || defined(HAVE_NETTLE)
  /* Security */
  rfb::Security security;

  /* Process security types which don't use encryption */
  if (encNoneCheckbox->value()) {
    if (authNoneCheckbox->value())
      security.EnableSecType(rfb::secTypeNone);
    if (authVncCheckbox->value()) {
      security.EnableSecType(rfb::secTypeVncAuth);
#ifdef HAVE_NETTLE
      security.EnableSecType(rfb::secTypeRA2ne);
      security.EnableSecType(rfb::secTypeRAne256);
#endif
    }
    if (authPlainCheckbox->value()) {
      security.EnableSecType(rfb::secTypePlain);
#ifdef HAVE_NETTLE
      security.EnableSecType(rfb::secTypeRA2ne);
      security.EnableSecType(rfb::secTypeRAne256);
      security.EnableSecType(rfb::secTypeDH);
      security.EnableSecType(rfb::secTypeMSLogonII);
#endif
    }
  }

#ifdef HAVE_GNUTLS
  /* Process security types which use TLS encryption */
  if (encTLSCheckbox->value()) {
    if (authNoneCheckbox->value())
      security.EnableSecType(rfb::secTypeTLSNone);
    if (authVncCheckbox->value())
      security.EnableSecType(rfb::secTypeTLSVnc);
    if (authPlainCheckbox->value())
      security.EnableSecType(rfb::secTypeTLSPlain);
  }

  /* Process security types which use X509 encryption */
  if (encX509Checkbox->value()) {
    if (authNoneCheckbox->value())
      security.EnableSecType(rfb::secTypeX509None);
    if (authVncCheckbox->value())
      security.EnableSecType(rfb::secTypeX509Vnc);
    if (authPlainCheckbox->value())
      security.EnableSecType(rfb::secTypeX509Plain);
  }

  rfb::CSecurityTLS::X509CA.setParam(caInput->value());
  rfb::CSecurityTLS::X509CRL.setParam(crlInput->value());
#endif

#ifdef HAVE_NETTLE
  if (encRSAAESCheckbox->value()) {
    security.EnableSecType(rfb::secTypeRA2);
    security.EnableSecType(rfb::secTypeRA256);
  }
#endif
  rfb::SecurityClient::secTypes.setParam(security.ToString());
#endif

  /* Input */
  viewOnly.setParam(viewOnlyCheckbox->value());
  emulateMiddleButton.setParam(emulateMBCheckbox->value());
  acceptClipboard.setParam(acceptClipboardCheckbox->value());
#if !defined(WIN32) && !defined(__APPLE__)
  setPrimary.setParam(setPrimaryCheckbox->value());
#endif
  sendClipboard.setParam(sendClipboardCheckbox->value());
#if !defined(WIN32) && !defined(__APPLE__)
  sendPrimary.setParam(sendPrimaryCheckbox->value());
#endif
  fullscreenSystemKeys.setParam(systemKeysCheckbox->value());

  if (menuKeyChoice->value() == 0)
    menuKey.setParam("None");
  else {
    menuKey.setParam(menuKeyChoice->text());
  }

  /* Display */
  if (windowedButton->value()) {
    fullScreen.setParam(false);
  } else {
    fullScreen.setParam(true);

    if (allMonitorsButton->value()) {
      fullScreenMode.setParam("All");
    } else if (selectedMonitorsButton->value()) {
      fullScreenMode.setParam("Selected");
    } else {
      fullScreenMode.setParam("Current");
    }
  }

  fullScreenSelectedMonitors.setMonitors(monitorArrangement->value());

  if (desktopSizeCheckbox->value() &&
      (strlen(desktopWidthInput->value()) > 0) &&
      (strlen(desktopHeightInput->value()) > 0)) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%sx%s", desktopWidthInput->value(),
             desktopHeightInput->value());
    desktopSize.setParam(buf);
  } else {
    desktopSize.setParam("");
  }

  remoteResize.setParam(remoteResizeCheckbox->value());

  /* Misc. */
  shared.setParam(sharedCheckbox->value());
  reconnectOnError.setParam(reconnectCheckbox->value());
  recordFile.setParam(recordInput->value());
  alwaysCursor.setParam(alwaysCursorCheckbox->value());

  if (cursorTypeChoice->value() == 1) {
    cursorType.setParam("System");
  } else {
    // Default
    cursorType.setParam("Dot");
  }

  std::map<OptionsCallback*, void*>::const_iterator iter;

  for (iter = callbacks.begin();iter != callbacks.end();++iter)
    iter->first(iter->second);
}


void OptionsDialog::createCompressionPage(int tx, int ty, int tw, int th)
{
  // TRANSLATORS: Tab heading for settings controlling image compression
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Compression"));

  int orig_tx, orig_ty;
  int col1_ty, col2_ty;
  int half_width, full_width;

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  full_width = tw - OUTER_MARGIN * 2;
  half_width = (full_width - INNER_MARGIN) / 2;

  /* AutoSelect checkbox */
  // TRANSLATORS: Option to automatically pick the best compression settings
  autoselectCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Auto select")));
  autoselectCheckbox->callback(handleAutoselect, this);
  ty += CHECK_HEIGHT + INNER_MARGIN;

  /* Two columns */
  orig_tx = tx;
  orig_ty = ty;

  /* VNC encoding box */
  ty += GROUP_LABEL_OFFSET;
  // TRANSLATORS: Label for a group of encoding method radio buttons
  encodingGroup = new Fl_Group(tx, ty, half_width, 0,
                                _("Preferred encoding"));
  encodingGroup->box(FL_FLAT_BOX);
  encodingGroup->labelfont(FL_BOLD);
  encodingGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += INDENT;
    ty += TIGHT_MARGIN;

    tightButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                               RADIO_MIN_WIDTH,
                                               RADIO_HEIGHT,
                                               "Tight"));
    tightButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    zrleButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                              RADIO_MIN_WIDTH,
                                              RADIO_HEIGHT,
                                              "ZRLE"));
    zrleButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    hextileButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                 RADIO_MIN_WIDTH,
                                                 RADIO_HEIGHT,
                                                 "Hextile"));
    hextileButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

#ifdef HAVE_H264
    h264Button = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                             RADIO_MIN_WIDTH,
                                             RADIO_HEIGHT,
                                             "H.264"));
    h264Button->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;
#endif
#ifdef HAVE_H265
    h265Button = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                             RADIO_MIN_WIDTH,
                                             RADIO_HEIGHT,
                                             "H.265"));
    h265Button->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;
#endif

    rawButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                             RADIO_MIN_WIDTH,
                                             RADIO_HEIGHT,
                                             "Raw"));
    rawButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;
  }

  ty -= TIGHT_MARGIN;

  encodingGroup->end();
  /* Needed for resize to work sanely */
  encodingGroup->resizable(nullptr);
  encodingGroup->size(encodingGroup->w(), ty - encodingGroup->y());
  col1_ty = ty;

  /* Second column */
  tx = orig_tx + half_width + INNER_MARGIN;
  ty = orig_ty;

  /* Color box */
  ty += GROUP_LABEL_OFFSET;
  // TRANSLATORS: Label for a group of color quality radio buttons
  colorlevelGroup = new Fl_Group(tx, ty, half_width, 0, _("Color level"));
  colorlevelGroup->labelfont(FL_BOLD);
  colorlevelGroup->box(FL_FLAT_BOX);
  colorlevelGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += INDENT;
    ty += TIGHT_MARGIN;

    // TRANSLATORS: Highest color quality
    fullcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                     RADIO_MIN_WIDTH,
                                                     RADIO_HEIGHT,
                                                     _("Full")));
    fullcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Medium color quality
    mediumcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                       RADIO_MIN_WIDTH,
                                                       RADIO_HEIGHT,
                                                       _("Medium")));
    mediumcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Low color quality
    lowcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                    RADIO_MIN_WIDTH,
                                                    RADIO_HEIGHT,
                                                    _("Low")));
    lowcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Very low color quality
    verylowcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                        RADIO_MIN_WIDTH,
                                                        RADIO_HEIGHT,
                                                        _("Very low")));
    verylowcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;
  }

  ty -= TIGHT_MARGIN;

  colorlevelGroup->end();
  /* Needed for resize to work sanely */
  colorlevelGroup->resizable(nullptr);
  colorlevelGroup->size(colorlevelGroup->w(),
                        ty - colorlevelGroup->y());
  col2_ty = ty;

  /* Back to normal */
  tx = orig_tx;
  ty = (col1_ty > col2_ty ? col1_ty : col2_ty) + INNER_MARGIN;

  /* Checkboxes */
  // TRANSLATORS: Enable manual entry of a compression level
  compressionCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Custom compression level:")));
  compressionCheckbox->labelfont(FL_BOLD);
  compressionCheckbox->callback(handleCompression, this);
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  // TRANSLATORS: Label for the numeric compression level input field
  compressionInput = new Fl_Int_Input(tx + INDENT, ty,
                                      INPUT_HEIGHT, INPUT_HEIGHT,
                                      _("level (0=fast, 9=best)"));
  compressionInput->align(FL_ALIGN_RIGHT);
  ty += INPUT_HEIGHT + INNER_MARGIN;

  // TRANSLATORS: Enable use of JPEG compression
  jpegCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                              CHECK_MIN_WIDTH,
                                              CHECK_HEIGHT,
                                              _("Allow JPEG compression:")));
  jpegCheckbox->labelfont(FL_BOLD);
  jpegCheckbox->callback(handleJpeg, this);
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  // TRANSLATORS: Label for JPEG quality numeric input field
  jpegInput = new Fl_Int_Input(tx + INDENT, ty,
                               INPUT_HEIGHT, INPUT_HEIGHT,
                               _("quality (0=poor, 9=best)"));
  jpegInput->align(FL_ALIGN_RIGHT);
  ty += INPUT_HEIGHT + INNER_MARGIN;

  group->end();
}


void OptionsDialog::createSecurityPage(int tx, int ty, int tw, int th)
{
#if defined(HAVE_GNUTLS) || defined(HAVE_NETTLE)
  // TRANSLATORS: Tab heading for encryption and authentication settings
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Security"));

  int orig_tx;
  int width;

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  width = tw - OUTER_MARGIN * 2;

  orig_tx = tx;

  /* Encryption */
  ty += GROUP_LABEL_OFFSET;
  // TRANSLATORS: Label for the encryption options section
  encryptionGroup = new Fl_Group(tx, ty, width, 0, _("Encryption"));
  encryptionGroup->labelfont(FL_BOLD);
  encryptionGroup->box(FL_FLAT_BOX);
  encryptionGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += INDENT;
    ty += TIGHT_MARGIN;

    // TRANSLATORS: No encryption will be used
    encNoneCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                   CHECK_MIN_WIDTH,
                                                   CHECK_HEIGHT,
                                                   _("None")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

#ifdef HAVE_GNUTLS
    // TRANSLATORS: Use TLS encryption without verifying the server
    encTLSCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("TLS with anonymous certificates")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Use TLS encryption with X509 certificates
    encX509Checkbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                   CHECK_MIN_WIDTH,
                                                   CHECK_HEIGHT,
                                                   _("TLS with X509 certificates")));
    encX509Checkbox->callback(handleX509, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    ty += INPUT_LABEL_OFFSET;
    // TRANSLATORS: Field for specifying the CA certificate file
    caInput = new Fl_Input(tx + INDENT, ty,
                           width - INDENT * 2, INPUT_HEIGHT,
                           _("Path to X509 CA certificate"));
    caInput->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    ty += INPUT_HEIGHT + TIGHT_MARGIN;

    ty += INPUT_LABEL_OFFSET;
    // TRANSLATORS: Field for specifying the certificate revocation list
    crlInput = new Fl_Input(tx + INDENT, ty,
                            width - INDENT * 2, INPUT_HEIGHT,
                            _("Path to X509 CRL file"));
    crlInput->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    ty += INPUT_HEIGHT + TIGHT_MARGIN;
#endif
#ifdef HAVE_NETTLE
    encRSAAESCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     "RSA-AES"));
    encRSAAESCheckbox->callback(handleRSAAES, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;
#endif
  }

  ty -= TIGHT_MARGIN;

  encryptionGroup->end();
  /* Needed for resize to work sanely */
  encryptionGroup->resizable(nullptr);
  encryptionGroup->size(encryptionGroup->w(),
                        ty - encryptionGroup->y());

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  /* Authentication */
  ty += GROUP_LABEL_OFFSET;
  // TRANSLATORS: Label for the authentication options section
  authenticationGroup = new Fl_Group(tx, ty, width, 0, _("Authentication"));
  authenticationGroup->labelfont(FL_BOLD);
  authenticationGroup->box(FL_FLAT_BOX);
  authenticationGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += INDENT;
    ty += TIGHT_MARGIN;

    // TRANSLATORS: No authentication required
    authNoneCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                    CHECK_MIN_WIDTH,
                                                    CHECK_HEIGHT,
                                                    _("None")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Traditional VNC password authentication
    authVncCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                   CHECK_MIN_WIDTH,
                                                   CHECK_HEIGHT,
                                                   _("Standard VNC (insecure without encryption)")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Authenticate using username and password
    authPlainCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Username and password (insecure without encryption)")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;
  }

  ty -= TIGHT_MARGIN;

  authenticationGroup->end();
  /* Needed for resize to work sanely */
  authenticationGroup->resizable(nullptr);
  authenticationGroup->size(authenticationGroup->w(),
                            ty - authenticationGroup->y());

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  group->end();
#else
  (void)tx;
  (void)ty;
  (void)tw;
  (void)th;
#endif
}


void OptionsDialog::createInputPage(int tx, int ty, int tw, int th)
{
  // TRANSLATORS: Tab heading for keyboard and mouse settings
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Input"));

  int orig_tx;
  int width;

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  width = tw - OUTER_MARGIN * 2;

  // TRANSLATORS: Disable sending mouse and keyboard events to the server
  viewOnlyCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("View only (ignore mouse and keyboard)")));
  ty += CHECK_HEIGHT + INNER_MARGIN;

  orig_tx = tx;

  /* Mouse */
  ty += GROUP_LABEL_OFFSET;
  // TRANSLATORS: Label for mouse related options
  mouseGroup = new Fl_Group(tx, ty, width, 0, _("Mouse"));
  mouseGroup->labelfont(FL_BOLD);
  mouseGroup->box(FL_FLAT_BOX);
  mouseGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += INDENT;
    ty += TIGHT_MARGIN;

    // TRANSLATORS: Option to emulate a middle mouse button
    emulateMBCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Emulate middle mouse button")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Show a local mouse cursor when the server does not provide one
    alwaysCursorCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                       CHECK_MIN_WIDTH,
                                                       CHECK_HEIGHT,
                                                       _("Show local cursor when not provided by server")));
    alwaysCursorCheckbox->callback(handleAlwaysCursor, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    /* Cursor type */
    // TRANSLATORS: Choice of cursor appearance used when showing a local cursor
    cursorTypeChoice = new Fl_Choice(LBLLEFT(tx, ty, 150, CHOICE_HEIGHT, _("Cursor type")));

    // TRANSLATORS: Simple dot shaped cursor
    fltk_menu_add(cursorTypeChoice, _("Dot"), 0, nullptr, nullptr, 0);
    // TRANSLATORS: Use the operating system's default cursor
    fltk_menu_add(cursorTypeChoice, _("System"), 0, nullptr, nullptr, 0);

    fltk_adjust_choice(cursorTypeChoice);

    ty += CHOICE_HEIGHT + TIGHT_MARGIN;

  }
  ty -= TIGHT_MARGIN;

  mouseGroup->end();
  /* Needed for resize to work sanely */
  mouseGroup->resizable(nullptr);
  mouseGroup->size(mouseGroup->w(), ty - mouseGroup->y());

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  /* Keyboard */
  ty += GROUP_LABEL_OFFSET;
  // TRANSLATORS: Label for keyboard related options
  keyboardGroup = new Fl_Group(tx, ty, width, 0, _("Keyboard"));
  keyboardGroup->labelfont(FL_BOLD);
  keyboardGroup->box(FL_FLAT_BOX);
  keyboardGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += INDENT;
    ty += TIGHT_MARGIN;

    // TRANSLATORS: Pass keys like Alt-Tab directly to the server when in full screen
    systemKeysCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                      CHECK_MIN_WIDTH,
                                                      CHECK_HEIGHT,
                                                      _("Pass system keys directly to server (full screen)")));
    systemKeysCheckbox->callback(handleSystemKeys, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Choice of key that will pop up the menu while full screen
    menuKeyChoice = new Fl_Choice(LBLLEFT(tx, ty, 150, CHOICE_HEIGHT, _("Menu key")));

    // TRANSLATORS: No menu key configured
    fltk_menu_add(menuKeyChoice, _("None"), 0, nullptr, nullptr, FL_MENU_DIVIDER);
    for (int idx = 0; idx < getMenuKeySymbolCount(); idx++)
      fltk_menu_add(menuKeyChoice, getMenuKeySymbols()[idx].name, 0, nullptr, nullptr, 0);

    fltk_adjust_choice(menuKeyChoice);

    ty += CHOICE_HEIGHT + TIGHT_MARGIN;
  }
  ty -= TIGHT_MARGIN;

  keyboardGroup->end();
  /* Needed for resize to work sanely */
  keyboardGroup->resizable(nullptr);
  keyboardGroup->size(keyboardGroup->w(), ty - keyboardGroup->y());

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  /* Clipboard */
  ty += GROUP_LABEL_OFFSET;
  // TRANSLATORS: Label for clipboard related settings
  clipboardGroup = new Fl_Group(tx, ty, width, 0, _("Clipboard"));
  clipboardGroup->labelfont(FL_BOLD);
  clipboardGroup->box(FL_FLAT_BOX);
  clipboardGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += INDENT;
    ty += TIGHT_MARGIN;

    // TRANSLATORS: Allow the server to update the local clipboard
    acceptClipboardCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                           CHECK_MIN_WIDTH,
                                                           CHECK_HEIGHT,
                                                           _("Accept clipboard from server")));
    acceptClipboardCheckbox->callback(handleClipboard, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

#if !defined(WIN32) && !defined(__APPLE__)
    // TRANSLATORS: Also update the primary selection on X11 systems
    setPrimaryCheckbox = new Fl_Check_Button(LBLRIGHT(tx + INDENT, ty,
                                                      CHECK_MIN_WIDTH,
                                                      CHECK_HEIGHT,
                                                      _("Also set primary selection")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;
#endif

    // TRANSLATORS: Send clipboard contents to the server
    sendClipboardCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                         CHECK_MIN_WIDTH,
                                                         CHECK_HEIGHT,
                                                         _("Send clipboard to server")));
    sendClipboardCheckbox->callback(handleClipboard, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

#if !defined(WIN32) && !defined(__APPLE__)
    // TRANSLATORS: Send the primary X11 selection as clipboard data
    sendPrimaryCheckbox = new Fl_Check_Button(LBLRIGHT(tx + INDENT, ty,
                                                       CHECK_MIN_WIDTH,
                                                       CHECK_HEIGHT,
                                                       _("Send primary selection as clipboard")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;
#endif
  }
  ty -= TIGHT_MARGIN;

  clipboardGroup->end();
  /* Needed for resize to work sanely */
  clipboardGroup->resizable(nullptr);
  clipboardGroup->size(clipboardGroup->w(), ty - clipboardGroup->y());

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  group->end();
}


void OptionsDialog::createDisplayPage(int tx, int ty, int tw, int th)
{
  // TRANSLATORS: Tab heading for display related settings
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Display"));

  int orig_tx;
  int width;

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  width = tw - OUTER_MARGIN * 2;

  orig_tx = tx;

  /* Display mode */
  ty += GROUP_LABEL_OFFSET;
  // TRANSLATORS: Label for the screen mode selection section
  displayModeGroup = new Fl_Group(tx, ty, width, 0, _("Display mode"));
  displayModeGroup->labelfont(FL_BOLD);
  displayModeGroup->box(FL_FLAT_BOX);
  displayModeGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += INDENT;
    ty += TIGHT_MARGIN;
    width -= INDENT;

    // TRANSLATORS: Run the viewer in a window
    windowedButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                  RADIO_MIN_WIDTH,
                                                  RADIO_HEIGHT,
                                                  _("Windowed")));
    windowedButton->type(FL_RADIO_BUTTON);
    windowedButton->callback(handleFullScreenMode, this);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Full screen on the monitor currently displaying the window
    currentMonitorButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                        RADIO_MIN_WIDTH,
                                                        RADIO_HEIGHT,
                                                        _("Full screen on current monitor")));
    currentMonitorButton->type(FL_RADIO_BUTTON);
    currentMonitorButton->callback(handleFullScreenMode, this);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Full screen spanning all monitors
    allMonitorsButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                            RADIO_MIN_WIDTH,
                                            RADIO_HEIGHT,
                                            _("Full screen on all monitors")));
    allMonitorsButton->type(FL_RADIO_BUTTON);
    allMonitorsButton->callback(handleFullScreenMode, this);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Full screen only on the monitors chosen below
    selectedMonitorsButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                 RADIO_MIN_WIDTH,
                                                 RADIO_HEIGHT,
                                                 _("Full screen on selected monitor(s)")));
    selectedMonitorsButton->type(FL_RADIO_BUTTON);
    selectedMonitorsButton->callback(handleFullScreenMode, this);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    monitorArrangement = new Fl_Monitor_Arrangement(
                              tx + INDENT, ty,
                              width - INDENT, 150);
    ty += 150 + TIGHT_MARGIN;
  }
  ty -= TIGHT_MARGIN;

  displayModeGroup->end();
  /* Needed for resize to work sanely */
  displayModeGroup->resizable(nullptr);
  displayModeGroup->size(displayModeGroup->w(),
                         ty - displayModeGroup->y());

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;
  width = tw - OUTER_MARGIN * 2;

  /* Desktop sizing */
  ty += GROUP_LABEL_OFFSET;
  // TRANSLATORS: Label for remote desktop sizing options
  desktopSizeGroup = new Fl_Group(tx, ty, width, 0, _("Desktop sizing"));
  desktopSizeGroup->labelfont(FL_BOLD);
  desktopSizeGroup->box(FL_FLAT_BOX);
  desktopSizeGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += INDENT;
    ty += TIGHT_MARGIN;
    width -= INDENT;

    // TRANSLATORS: Request a desktop resize when connecting
    desktopSizeCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                    CHECK_MIN_WIDTH,
                                                    CHECK_HEIGHT,
                                                    _("Resize remote session on connect")));
    desktopSizeCheckbox->callback(handleDesktopSize, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    desktopWidthInput = new Fl_Int_Input(tx + INDENT, ty,
                                         50, INPUT_HEIGHT);
    desktopWidthInput->align(FL_ALIGN_RIGHT);
    Fl_Box *xLabel = new Fl_Box(desktopWidthInput->x() + desktopWidthInput->w(), ty,
                                gui_str_len(" x "), INPUT_HEIGHT, " x ");
    desktopHeightInput = new Fl_Int_Input(xLabel->x() + xLabel->w(), ty,
                                          50, INPUT_HEIGHT);
    desktopHeightInput->align(FL_ALIGN_RIGHT);
    ty += INPUT_HEIGHT + TIGHT_MARGIN;

    // TRANSLATORS: Dynamically resize the remote desktop when the window size changes
    remoteResizeCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Resize remote session to the local window")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;
  }
  ty -= TIGHT_MARGIN;

  desktopSizeGroup->end();
  desktopSizeGroup->resizable(nullptr);
  desktopSizeGroup->size(desktopSizeGroup->w(),
                         ty - desktopSizeGroup->y());

  group->end();
}


void OptionsDialog::createMiscPage(int tx, int ty, int tw, int th)
{
  // TRANSLATORS: Tab heading for various other settings
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Miscellaneous"));

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  // TRANSLATORS: Allow multiple viewers to be connected simultaneously
  sharedCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("Shared (don't disconnect other viewers)")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  // TRANSLATORS: Ask whether to reconnect if the connection drops
  reconnectCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("Ask to reconnect on connection errors")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  // TRANSLATORS: File to record the session to
  recordInput = new Fl_Input(LBLRIGHT(tx, ty,
                                          CHECK_MIN_WIDTH,
                                          CHECK_HEIGHT,
                                          _("Record file")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  group->end();
}


void OptionsDialog::handleAutoselect(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->autoselectCheckbox->value()) {
    dialog->encodingGroup->deactivate();
    dialog->colorlevelGroup->deactivate();
  } else {
    dialog->encodingGroup->activate();
    dialog->colorlevelGroup->activate();
  }

  // JPEG setting is also affected by autoselection
  dialog->handleJpeg(dialog->jpegCheckbox, dialog);
}


void OptionsDialog::handleCompression(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->compressionCheckbox->value())
    dialog->compressionInput->activate();
  else
    dialog->compressionInput->deactivate();
}


void OptionsDialog::handleJpeg(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->jpegCheckbox->value() &&
      !dialog->autoselectCheckbox->value())
    dialog->jpegInput->activate();
  else
    dialog->jpegInput->deactivate();
}


void OptionsDialog::handleX509(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->encX509Checkbox->value()) {
    dialog->caInput->activate();
    dialog->crlInput->activate();
  } else {
    dialog->caInput->deactivate();
    dialog->crlInput->deactivate();
  }
}


void OptionsDialog::handleRSAAES(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->encRSAAESCheckbox->value()) {
    dialog->authVncCheckbox->value(true);
    dialog->authPlainCheckbox->value(true);
  }
}


void OptionsDialog::handleSystemKeys(Fl_Widget* /*widget*/, void* data)
{
#ifdef __APPLE__
  OptionsDialog* dialog = (OptionsDialog*)data;

  // Pop up the access dialog if needed
  if (dialog->systemKeysCheckbox->value())
    cocoa_is_trusted(true);
#else
  (void)data;
#endif
}


void OptionsDialog::handleDesktopSize(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->desktopSizeCheckbox->value()) {
    dialog->desktopWidthInput->activate();
    dialog->desktopHeightInput->activate();
  } else {
    dialog->desktopWidthInput->deactivate();
    dialog->desktopHeightInput->deactivate();
  }
}


void OptionsDialog::handleClipboard(Fl_Widget* /*widget*/, void *data)
{
  (void)data;
#if !defined(WIN32) && !defined(__APPLE__)
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->acceptClipboardCheckbox->value())
    dialog->setPrimaryCheckbox->activate();
  else
    dialog->setPrimaryCheckbox->deactivate();
  if (dialog->sendClipboardCheckbox->value())
    dialog->sendPrimaryCheckbox->activate();
  else
    dialog->sendPrimaryCheckbox->deactivate();
#endif
}

void OptionsDialog::handleFullScreenMode(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->selectedMonitorsButton->value()) {
    dialog->monitorArrangement->activate();
  } else {
    dialog->monitorArrangement->deactivate();
  }
}

void OptionsDialog::handleCancel(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  dialog->hide();
}


void OptionsDialog::handleOK(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  dialog->hide();

  dialog->storeOptions();
}

int OptionsDialog::fltk_event_handler(int event)
{
  std::set<OptionsDialog *>::iterator iter;

  if (event != FL_SCREEN_CONFIGURATION_CHANGED)
    return 0;

  // Refresh monitor arrangement widget to match the parameter settings after
  // screen configuration has changed. The MonitorArrangement index doesn't work
  // the same way as the FLTK screen index.
  for (iter = instances.begin(); iter != instances.end(); iter++)
      Fl::add_timeout(0, handleScreenConfigTimeout, (*iter));

  return 0;
}

void OptionsDialog::handleScreenConfigTimeout(void *data)
{
    OptionsDialog *self = (OptionsDialog *)data;

    assert(self);

    self->monitorArrangement->value(fullScreenSelectedMonitors.getMonitors());
}

void OptionsDialog::handleAlwaysCursor(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->alwaysCursorCheckbox->value()) {
    dialog->cursorTypeChoice->activate();
  } else {
    dialog->cursorTypeChoice->deactivate();
  }
}
