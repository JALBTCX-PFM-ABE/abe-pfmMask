
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.
*********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! or / / ! are being used by Doxygen to
    document the software.  Dashes in these comment blocks are used to create bullet lists.
    The lack of blank lines after a block of dash preceeded comments means that the next
    block of dash preceeded comments is a new, indented bullet list.  I've tried to keep the
    Doxygen formatting to a minimum but there are some other items (like <br> and <pre>)
    that need to be left alone.  If you see a comment that starts with / * ! or / / ! and
    there is something that looks a bit weird it is probably due to some arcane Doxygen
    syntax.  Be very careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/



#include "pfmMask.hpp"
#include "pfmMaskHelp.hpp"


double settings_version = 1.0;


pfmMask::pfmMask (int32_t *argc, char **argv, QWidget *parent)
  : QWizard (parent, 0)
{
  QResource::registerResource ("/icons.rcc");


  //  Set the main icon

  setWindowIcon (QIcon (":/icons/pfmMaskWatermark.png"));


  //  Get the user's defaults if available

  envin (&options);


  // Set the application font

  QApplication::setFont (options.font);


  setWizardStyle (QWizard::ClassicStyle);


  setOption (HaveHelpButton, true);
  setOption (ExtendedWatermarkPixmap, false);

  connect (this, SIGNAL (helpRequested ()), this, SLOT (slotHelpClicked ()));


  //  Set the window size and location from the defaults

  this->resize (options.window_width, options.window_height);
  this->move (options.window_x, options.window_y);


  setPage (0, new startPage (argc, argv, &options, this));

  setPage (1, new runPage (this, &progress, &checkList));


  setButtonText (QWizard::CustomButton1, tr("&Run"));
  setOption (QWizard::HaveCustomButton1, true);
  button (QWizard::CustomButton1)->setToolTip (tr ("Start masking the PFM file"));
  button (QWizard::CustomButton1)->setWhatsThis (runText);
  connect (this, SIGNAL (customButtonClicked (int)), this, SLOT (slotCustomButtonClicked (int)));


  setStartId (0);
}


pfmMask::~pfmMask ()
{
}



void pfmMask::initializePage (int id)
{
  button (QWizard::HelpButton)->setIcon (QIcon (":/icons/contextHelp.png"));
  button (QWizard::CustomButton1)->setEnabled (false);


  switch (id)
    {
    case 0:
      break;

    case 1:

      options.topo = field ("topo").toBool ();
      options.mask = field ("mask").toDouble ();
      mask = (float) options.mask;


      //  Check the mask value.

      if (options.mask < options.min_z || options.mask > options.max_z)
        {
          QMessageBox::critical (this, tr ("pfmMask"),
                                 tr ("The mask value (%1) is outside of the PFM Z bounds (%2 to %3).  Please correct.").arg
                                 (mask, 0, 'f', 2).arg (options.min_z, 0, 'f', 2).arg (options.max_z, 0, 'f', 2));
        }
      else
        {
          button (QWizard::CustomButton1)->setEnabled (true);

          pfm_file_name = field ("pfm_file_edit").toString ();

          bit_set (&mask, 0, 0);


          //  Use frame geometry to get the absolute x and y.

          QRect tmp = this->frameGeometry ();
          options.window_x = tmp.x ();
          options.window_y = tmp.y ();


          //  Use geometry to get the width and height.

          tmp = this->geometry ();
          options.window_width = tmp.width ();
          options.window_height = tmp.height ();


          //  Save the options.

          envout (&options);


          QString string;

          checkList->clear ();

          string = tr ("Input PFM file : ") + pfm_file_name;
          checkList->addItem (string);

          if (options.topo)
            {
              string = tr ("Using SRTM topo data");
            }
          else
            {
              string.sprintf (tr ("Mask value : %.2f").toLatin1 (), mask);
            }
          checkList->addItem (string);
        }
      break;
    }
}



void pfmMask::cleanupPage (int id)
{
  switch (id)
    {
    case 0:
      break;

    case 1:
      break;
    }
}



void pfmMask::slotHelpClicked ()
{
  QWhatsThis::enterWhatsThisMode ();
}



//  This is where the fun stuff happens.

void 
pfmMask::slotCustomButtonClicked (int id __attribute__ ((unused)))
{
  int32_t             pfm_handle, width, height;
  uint8_t             misp = NVFalse;
  BIN_RECORD          bin;
  PFM_OPEN_ARGS       open_args;
  QString             string;



  QApplication::setOverrideCursor (Qt::WaitCursor);


  button (QWizard::FinishButton)->setEnabled (false);
  button (QWizard::BackButton)->setEnabled (false);
  button (QWizard::CustomButton1)->setEnabled (false);


  strcpy (open_args.list_path, pfm_file_name.toLatin1 ());


  progress.mbox->setTitle (tr ("Creating checkpoint file"));
  progress.mbar->setRange (0, 0);
  qApp->processEvents ();


  //  Check point the file in case we barf.

  open_args.checkpoint = 1;
  pfm_handle = open_existing_pfm_file (&open_args);


  //  We're going to try to use PFM_USER_10 as a landmask tag (assuming it hasn't been used yet).

  if (strcmp (open_args.head.user_flag_name[9], "PFM_USER_10") && strcmp (open_args.head.user_flag_name[9], "Land masked point"))
    {
      QMessageBox::critical (this, tr ("pfmMask"), 
                             tr ("Unable to use PFM_USER_10 flag for land masked data.\nFlag already in use for %1").arg (open_args.head.user_flag_name[9]));
      exit (-1);
    }

  strcpy (open_args.head.user_flag_name[9], "Land masked point");

  write_bin_header (pfm_handle, &open_args.head, NVFalse);


  width = open_args.head.bin_width;
  height = open_args.head.bin_height;


  //  Check to see if the land mask is available.

  if (options.topo)
    {
      //  Just to keep life simple I'm excluding the srtm2 data (DOD restricted).  Since we only use this for 
      //  large scale areas it shouldn't matter.

      set_exclude_srtm2_data (NVTrue);
    }
  else
    {
      if (check_swbd_mask (1) != NULL)
        {
          QString err = QString (check_swbd_mask (1));

          QMessageBox::critical (this, tr ("pfmMask"), tr ("The SWBD mask is not avalable for the following reason : \n\n") + err);

          exit (-1);
        }
    }


  //  Check to see if the average surface is a MISP or GMT surface.

  if (strstr (open_args.head.average_filt_name, "MINIMUM MISP") || strstr (open_args.head.average_filt_name, "AVERAGE MISP") ||
      strstr (open_args.head.average_filt_name, "MAXIMUM MISP") || strstr (open_args.head.average_filt_name, "MINIMUM GMT") ||
      strstr (open_args.head.average_filt_name, "AVERAGE GMT") || strstr (open_args.head.average_filt_name, "MAXIMUM GMT"))
    misp = NVTrue;


  int32_t decon = 0;
  int32_t mask_file = 0;


  //  Check to see if we already have SRTM data in the PFM file.

  int32_t file_count = get_next_list_file_number (pfm_handle);
  int32_t line_count = get_next_line_number (pfm_handle);

  for (int16_t i = 0 ; i < file_count ; i++)
    {
      char filename[512];
      int16_t type;

      read_list_file (pfm_handle, i, filename, &type);


      if (strstr (filename, "SRTM_mask")) mask_file = i;


      if (strstr (filename, "SRTM_data"))
        {
          QMessageBox msgBox (this);
          msgBox.setIcon (QMessageBox::Question);
          msgBox.setInformativeText (tr ("SRTM data is already loaded in this PFM.  Do you wish to deconflict it with the input data?"));
          msgBox.setStandardButtons (QMessageBox::Yes | QMessageBox::No);
          msgBox.setDefaultButton (QMessageBox::Yes);
          int32_t ret = msgBox.exec ();

          if (ret == QMessageBox::Yes)
            {
              decon = i;
              break;
            }
          else
            {
              break;
            }
        }
    }


  if (mask_file)
    {
      decon = 0;
      file_count = mask_file;
    }


  if (decon)
    {
      progress.mbox->setTitle (tr ("Deconflicting SRTM data with input data"));
    }
  else
    {
      progress.mbox->setTitle (tr ("Filling land data"));
    }
  progress.mbar->setRange (0, height);
  qApp->processEvents ();


  double half_x = open_args.head.x_bin_size_degrees / 2.0, half_y = open_args.head.y_bin_size_degrees / 2.0;
  uint8_t add_file = NVFalse;


  for (int32_t i = 0 ; i < height ; i++)
    {
      NV_F64_COORD2 nxy;


      nxy.y = open_args.head.mbr.min_y + (double) i * open_args.head.y_bin_size_degrees + half_y;

      for (int32_t j = 0 ; j < width ; j++)
	{
          nxy.x = open_args.head.mbr.min_x + (double) j * open_args.head.x_bin_size_degrees + half_x;


          //  Don't try to deal with points that fall outside of the PFM polygon (it might not be a rectangle).

          if (bin_inside_ptr (&open_args.head, nxy))
            {
              NV_I32_COORD2 coord;
              compute_index_ptr (nxy, &coord, &open_args.head);
              read_bin_record_index (pfm_handle, coord, &bin);


              int32_t recnum;


              //  First case, we have SRTM elevation data loaded in the PFM.  We need to deconflict it with the normal input data.

              if (decon)
                {
                  DEPTH_RECORD *dep;

                  if (bin.validity & PFM_DATA)
                    {
                      if (!read_depth_array_index (pfm_handle, coord, &dep, &recnum))
                        {
                          uint8_t valid = NVFalse, srtm = NVFalse;

                          for (int32_t k = 0 ; k < recnum ; k++)
                            {
                              if (!(dep[k].validity & (PFM_INVAL | PFM_DELETED)))
                                {
                                  if (dep[k].file_number == decon)
                                    {
                                      srtm = NVTrue;
                                      if (valid) break;
                                    }
                                  else
                                    {
                                      valid = NVTrue;
                                      if (srtm) break;
                                    }
                                }
                            }


                          //  If we had SRTM elevation data and valid normal data we need to invalidate the SRTM data.

                          if (srtm && valid)
                            {
                              for (int32_t k = 0 ; k < recnum ; k++)
                                {
                                  if (!(dep[k].validity & (PFM_INVAL | PFM_DELETED)))
                                    {
                                      if (dep[k].file_number == decon)
                                        {
                                          dep[k].validity |= PFM_FILTER_INVAL;


                                          //  Update the depth record.

                                          int32_t status = update_depth_record_index (pfm_handle, &dep[k]);
                                          if (status != SUCCESS)
                                            {
                                              fprintf (stderr, "Error on depth status update.\n");
                                              fprintf (stderr, "%s\n", pfm_error_str (status));
                                              fflush (stderr);
                                            }


                                          //  Recompute the bin record based on the modified contents of the depth array.

                                          recompute_bin_values_index (pfm_handle, coord, &bin, 0);
                                        }
                                    }
                                }
                            }

                          free (dep);
                        }
                    }
                }


              //  Second case, we have already run pfmMask on the file but we (probably) want to change the elevation level of the mask value.
              //  We add the mask to empty "land" cells and replace existing mask values where there is no normal input data.

              else if (mask_file)
                {
                  //  There is data in the cell (may be mask or normal).

                  if (bin.validity & PFM_DATA)
                    {
                      DEPTH_RECORD *dep;

                      if (!read_depth_array_index (pfm_handle, coord, &dep, &recnum))
                        {
                          uint8_t valid = NVFalse, srtm = NVFalse;

                          for (int32_t k = 0 ; k < recnum ; k++)
                            {
                              if (!(dep[k].validity & (PFM_INVAL | PFM_DELETED)))
                                {
                                  if (dep[k].file_number == mask_file)
                                    {
                                      srtm = NVTrue;
                                      if (valid) break;
                                    }
                                  else
                                    {
                                      valid = NVTrue;
                                      if (srtm) break;
                                    }
                                }
                            }


                          //  If we only had SRTM mask or elevation values, replace the depth value.

                          if (srtm && !valid)
                            {
                              float value = 0.0;

                              for (int32_t k = 0 ; k < recnum ; k++)
                                {
                                  if (!(dep[k].validity & (PFM_INVAL | PFM_DELETED)))
                                    {
                                      if (dep[k].file_number == mask_file)
                                        {
                                          dep[k].xyz.z = 0.0;

                                          if (options.topo)
                                            {
                                              int16_t elev = read_srtm_topo (nxy.y, nxy.x);
                                              if (elev && elev > 0 && elev != 32767) dep[k].xyz.z = -((float) elev);
                                            }
                                          else
                                            {
                                              if (swbd_is_land (nxy.y, nxy.x, 1)) dep[k].xyz.z = (float) mask;
                                            }

                                          if (dep[k].xyz.z != 0.0)
                                            {
                                              value = dep[k].xyz.z;

                                              dep[k].validity = PFM_USER_05 | PFM_MODIFIED;


                                              //  Update the depth array record.

                                              int32_t status = change_depth_record_index (pfm_handle, &dep[k]);
                                              if (status != SUCCESS)
                                                {
                                                  fprintf (stderr, "Error on depth status update.\n");
                                                  fprintf (stderr, "%s\n", pfm_error_str (status));
                                                  fflush (stderr);
                                                }
                                            }
                                        }
                                    }
                                }


                              //  If this was a MISP or GMT surface we have to manually replace the average surface with the
                              //  mask value.

                              if (misp)
                                {
                                  //  We have to re-read the bin record because the update_depth_record changed the bin record.

                                  read_bin_record_index (pfm_handle, coord, &bin);


                                  bin.avg_filtered_depth = value;


                                  //  Write the record back out.

                                  write_bin_record_index (pfm_handle, &bin);
                                }


                              //  Recompute the bin record based on the modified contents of the depth array.

                              recompute_bin_values_index (pfm_handle, coord, &bin, 0);
                            }

                          free (dep);
                        }
                    }


                  //  This is an empty cell so we need to mask it if it's land.

                  else
                    {
                      DEPTH_RECORD dep;

                      dep.xyz.z = 0.0;

                      if (options.topo)
                        {
                          int16_t elev = read_srtm_topo (nxy.y, nxy.x);
                          if (elev && elev > 0 && elev != 32767) dep.xyz.z = -((float) elev);
                        }
                      else
                        {
                          if (swbd_is_land (nxy.y, nxy.x, 1)) dep.xyz.z = (float) mask;
                        }


                      if (dep.xyz.z != 0.0)
                        {
                          dep.xyz.x = nxy.x;
                          dep.xyz.y = nxy.y;
                          dep.horizontal_error = -999.0;
                          dep.vertical_error = -999.0;
                          dep.coord = coord;

                          dep.validity = PFM_USER_05 | PFM_MODIFIED;
                          dep.beam_number = 0;
                          dep.ping_number = 0;
                          dep.line_number = line_count;
                          dep.file_number = file_count;


                          //  Add the mask value at the center of the bin as a depth record.

                          int32_t status = add_depth_record_index (pfm_handle, &dep);

                          if (status) pfm_error_exit (status);


                          //  If this was a MISP or GMT surface we have to manually replace the average surface with the
                          //  mask value.

                          if (misp)
                            {
                              //  We have to re-read the bin record because the add_depth_record changed the bin record.

                              read_bin_record_index (pfm_handle, coord, &bin);


                              bin.avg_filtered_depth = dep.xyz.z;


                              //  Write the record back out.

                              write_bin_record_index (pfm_handle, &bin);
                            }


                          //  Recompute the bin record based on the modified contents of the depth array.

                          recompute_bin_values_index (pfm_handle, coord, &bin, 0);
                        }
                    }
                }


              //  Final case, neither SRTM elevations or previous masks were in the PFM so we just want to mask the land.

              else
                {
                  DEPTH_RECORD dep;


                  //  Only put mask points in bins without any valid data.

                  if (!(bin.validity & PFM_DATA))
                    {
                      dep.xyz.z = 0.0;

                      if (options.topo)
                        {
                          int16_t elev = read_srtm_topo (nxy.y, nxy.x);
                          if (elev && elev > 0 && elev != 32767) dep.xyz.z = -((float) elev);
                        }
                      else
                        {
                          if (swbd_is_land (nxy.y, nxy.x, 1)) dep.xyz.z = (float) mask;
                        }


                      if (dep.xyz.z != 0.0)
                        {
                          dep.xyz.x = nxy.x;
                          dep.xyz.y = nxy.y;
                          dep.horizontal_error = -999.0;
                          dep.vertical_error = -999.0;
                          dep.coord = coord;


                          dep.validity = PFM_USER_05 | PFM_MODIFIED;
                          dep.beam_number = 0;
                          dep.ping_number = 0;
                          dep.line_number = line_count;
                          dep.file_number = file_count;


                          add_file = NVTrue;


                          //  Add the mask value at the center of the bin as a depth record.

                          int32_t status = add_depth_record_index (pfm_handle, &dep);

                          if (status) pfm_error_exit (status);


                          //  If this was a MISP or GMT surface we have to manually replace the average surface with the
                          //  mask value.

                          if (misp)
                            {
                              //  We have to re-read the bin record because the add_depth_record changed the bin record.

                              read_bin_record_index (pfm_handle, coord, &bin);


                              bin.avg_filtered_depth = dep.xyz.z;


                              //  Write the record back out.

                              write_bin_record_index (pfm_handle, &bin);
                            }


                          //  Recompute the bin record based on the modified contents of the depth array.

                          recompute_bin_values_index (pfm_handle, coord, &bin, 0);
                        }
                    }
                }
            }

          progress.mbar->setValue (i);
          qApp->processEvents ();
        }
    }

  progress.mbar->setValue (height);
  qApp->processEvents ();


  //if (!decon) swbd_is_land (999.0, 999.0, 1);


  checkList->clear ();


  if (add_file && !mask_file)
    {
      write_line_file (pfm_handle, (char *) "SRTM_mask");
      write_list_file (pfm_handle, (char *) "/SRTM_mask", PFM_NAVO_ASCII_DATA);
    }


  close_pfm_file (pfm_handle);


  button (QWizard::FinishButton)->setEnabled (true);
  button (QWizard::CancelButton)->setEnabled (false);


  QApplication::restoreOverrideCursor ();


  checkList->addItem (" ");
  QListWidgetItem *cur = new QListWidgetItem (tr ("Masking complete, press Finish to exit."));

  checkList->addItem (cur);
  checkList->setCurrentItem (cur);
  checkList->scrollToItem (cur);
}



//  Get the users defaults.

void pfmMask::envin (OPTIONS *options)
{
  //  We need to get the font from the global settings.

#ifdef NVWIN3X
  QString ini_file2 = QString (getenv ("USERPROFILE")) + "/ABE.config/" + "globalABE.ini";
#else
  QString ini_file2 = QString (getenv ("HOME")) + "/ABE.config/" + "globalABE.ini";
#endif

  options->font = QApplication::font ();

  QSettings settings2 (ini_file2, QSettings::IniFormat);
  settings2.beginGroup ("globalABE");


  QString defaultFont = options->font.toString ();
  QString fontString = settings2.value (QString ("ABE map GUI font"), defaultFont).toString ();
  options->font.fromString (fontString);


  settings2.endGroup ();


  double saved_version = 1.0;


  // Set defaults so that if keys don't exist the parameters are defined

  options->topo = NVFalse;
  options->mask = -5.0;
  options->input_dir = ".";
  options->window_x = 0;
  options->window_y = 0;
  options->window_width = 1000;
  options->window_height = 500;


  //  Get the INI file name

#ifdef NVWIN3X
  QString ini_file = QString (getenv ("USERPROFILE")) + "/ABE.config/pfmMask.ini";
#else
  QString ini_file = QString (getenv ("HOME")) + "/ABE.config/pfmMask.ini";
#endif

  QSettings settings (ini_file, QSettings::IniFormat);
  settings.beginGroup ("pfmMask");

  saved_version = settings.value (QString ("settings version"), saved_version).toDouble ();


  //  If the settings version has changed we need to leave the values at the new defaults since they may have changed.

  if (settings_version != saved_version) return;


  options->topo = settings.value (QString ("topo"), options->topo).toBool ();

  options->mask = settings.value (QString ("mask"), options->mask).toDouble ();

  options->input_dir = settings.value (QString ("input directory"), options->input_dir).toString ();

  options->window_width = settings.value (QString ("width"), options->window_width).toInt ();
  options->window_height = settings.value (QString ("height"), options->window_height).toInt ();
  options->window_x = settings.value (QString ("x position"), options->window_x).toInt ();
  options->window_y = settings.value (QString ("y position"), options->window_y).toInt ();

  settings.endGroup ();
}




//  Save the users defaults.

void pfmMask::envout (OPTIONS *options)
{
  //  Get the INI file name

#ifdef NVWIN3X
  QString ini_file = QString (getenv ("USERPROFILE")) + "/ABE.config/pfmMask.ini";
#else
  QString ini_file = QString (getenv ("HOME")) + "/ABE.config/pfmMask.ini";
#endif

  QSettings settings (ini_file, QSettings::IniFormat);
  settings.beginGroup ("pfmMask");


  settings.setValue (QString ("settings version"), settings_version);

  settings.setValue (QString ("topo"), options->topo);

  settings.setValue (QString ("mask"), options->mask);

  settings.setValue (QString ("input directory"), options->input_dir);

  settings.setValue (QString ("width"), options->window_width);
  settings.setValue (QString ("height"), options->window_height);
  settings.setValue (QString ("x position"), options->window_x);
  settings.setValue (QString ("y position"), options->window_y);

  settings.endGroup ();
}
