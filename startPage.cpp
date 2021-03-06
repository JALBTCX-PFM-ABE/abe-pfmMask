
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



#include "startPage.hpp"
#include "startPageHelp.hpp"

startPage::startPage (int32_t *argc, char **argv, OPTIONS *op, QWidget *parent):
  QWizardPage (parent)
{
  options = op;


  setPixmap (QWizard::WatermarkPixmap, QPixmap(":/icons/pfmMaskWatermark.png"));


  setTitle (tr ("Introduction"));

  setWhatsThis (tr ("See, it really works!"));

  QLabel *label = new QLabel (tr ("pfmMask is a tool for masking land areas in a PFM file using the SWBD "
                                  "1 second land mask.  Help is available by clicking on the Help button and then "
                                  "clicking on the item for which you want help.  Select a PFM file below.  "
                                  "Click <b>Next</b> to continue or <b>Cancel</b> to exit."));
  label->setWordWrap (true);


  QVBoxLayout *vbox = new QVBoxLayout (this);
  vbox->addWidget (label);
  vbox->addStretch (10);


  QHBoxLayout *pfm_file_box = new QHBoxLayout (0);
  pfm_file_box->setSpacing (8);

  vbox->addLayout (pfm_file_box);


  QLabel *pfm_file_label = new QLabel (tr ("PFM File"), this);
  pfm_file_box->addWidget (pfm_file_label, 1);

  pfm_file_edit = new QLineEdit (this);
  pfm_file_edit->setReadOnly (true);
  pfm_file_box->addWidget (pfm_file_edit, 10);

  QPushButton *pfm_file_browse = new QPushButton (tr ("Browse..."), this);
  pfm_file_box->addWidget (pfm_file_browse, 1);

  pfm_file_label->setWhatsThis (pfm_fileText);
  pfm_file_edit->setWhatsThis (pfm_fileText);
  pfm_file_browse->setWhatsThis (pfm_fileBrowseText);

  connect (pfm_file_browse, SIGNAL (clicked ()), this, SLOT (slotPFMFileBrowse ()));


  QGroupBox *tBox = new QGroupBox (tr ("Use SRTM topo data"), this);
  QHBoxLayout *tBoxLayout = new QHBoxLayout;
  tBox->setLayout (tBoxLayout);
  topo = new QCheckBox (tBox);
  topo->setToolTip (tr ("Use SRTM topo data values instead of SWBD land mask fixed values"));
  topo->setWhatsThis (topoText);
  tBoxLayout->addWidget (topo);
  if (!check_srtm3_topo ())
    {
      options->topo = NVFalse;
      topo->setEnabled (false);
    }
  topo->setChecked (options->topo);


  connect (topo, SIGNAL (clicked ()), this, SLOT (slotTopoClicked (void)));

  vbox->addWidget (tBox);


  QGroupBox *maskBox = new QGroupBox (tr ("Mask value"), this);
  QHBoxLayout *maskBoxLayout = new QHBoxLayout;
  maskBox->setLayout (maskBoxLayout);

  mask = new QDoubleSpinBox (this);
  mask->setDecimals (1);
  mask->setRange (-12000.0, 12000.0);
  mask->setSingleStep (100.0);
  mask->setValue (options->mask);
  mask->setWrapping (true);
  mask->setToolTip (tr ("Change the land mask value (-12000.0 to 12000.0)"));
  mask->setWhatsThis (maskText);
  maskBoxLayout->addWidget (mask);
  if (options->topo) mask->setEnabled (false);
  vbox->addWidget (maskBox);


  if (*argc == 2)
    {
      PFM_OPEN_ARGS open_args;
      int32_t pfm_handle = -1;


      QString pfm_file_name = QString (argv[1]);


      strcpy (open_args.list_path, pfm_file_name.toLatin1 ());

      open_args.checkpoint = 0;
      pfm_handle = open_existing_pfm_file (&open_args);

      if (pfm_handle >= 0)
        {
          //  Save the min and max values so we don't try to insert a mask value that is outside the bounds.

          options->min_z = -open_args.offset;
          options->max_z = open_args.max_depth;


	  pfm_file_edit->setText (pfm_file_name);

	  close_pfm_file (pfm_handle);
        }
    }


  if (!pfm_file_edit->text ().isEmpty ())
    {
      registerField ("pfm_file_edit", pfm_file_edit);
    }
  else
    {
      registerField ("pfm_file_edit*", pfm_file_edit);
    }
  registerField ("topo", topo);
  registerField ("mask", mask, "value");
}



void startPage::slotPFMFileBrowse ()
{
  PFM_OPEN_ARGS       open_args;
  QStringList         files, filters;
  QString             file;
  int32_t             pfm_handle = -1;


  QFileDialog *fd = new QFileDialog (this, tr ("pfmMask Open PFM File"));
  fd->setViewMode (QFileDialog::List);


  //  Always add the current working directory and the last used directory to the sidebar URLs in case we're running from the command line.
  //  This function is in the nvutility library.

  setSidebarUrls (fd, options->input_dir);


  filters << tr ("PFM (*.pfm)");

  fd->setNameFilters (filters);
  fd->setFileMode (QFileDialog::ExistingFile);
  fd->selectNameFilter (tr ("PFM (*.pfm)"));

  if (fd->exec () == QDialog::Accepted)
    {
      files = fd->selectedFiles ();

      QString pfm_file_name = files.at (0);


      if (!pfm_file_name.isEmpty())
        {
          strcpy (open_args.list_path, pfm_file_name.toLatin1 ());

          open_args.checkpoint = 0;
          pfm_handle = open_existing_pfm_file (&open_args);

          if (pfm_handle < 0)
            {
              QMessageBox::warning (this, tr ("Open PFM File"),
				    tr ("The file ") + QDir::toNativeSeparators (pfm_file_name) + 
				    tr (" is not a PFM file or there was an error reading the file.") +
				    tr ("  The error message returned was:\n\n") +
				    QString (pfm_error_str (pfm_error)));

	      return;
            }


          //  Save the min and max values so we don't try to insert a mask value that is outside the bounds.

          options->min_z = -open_args.offset;
          options->max_z = open_args.max_depth;


	  close_pfm_file (pfm_handle);
        }


      pfm_file_edit->setText (pfm_file_name);

      options->input_dir = fd->directory ().absolutePath ();
    }
}



void 
startPage::slotTopoClicked ()
{
  if (topo->checkState ())
    {
      options->topo = NVTrue;
      mask->setEnabled (false);
    }
  else
    {
      options->topo = NVFalse;
      mask->setEnabled (true);
    }
}
