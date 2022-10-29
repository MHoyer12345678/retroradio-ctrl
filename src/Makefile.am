ACLOCAL_AMFLAGS=-I m4

bin_PROGRAMS=retroradio-controller

retroradio_controller_SOURCES =	\
	main.cpp						\
	RetroradioControllerConfiguration.cpp			\
	RetroradioControllerConfiguration.h				\
	RetroradioController.cpp						\
	RetroradioController.h							\
	AbstractPersistentState.cpp						\
	AbstractPersistentState.h						\
	RetroradioPersistentState.cpp					\
	RetroradioPersistentState.h						\
	AudioController.cpp								\
	AudioController.h								\
	BasicMixerControl.cpp							\
	BasicMixerControl.h								\
	MainVolumeControl.cpp							\
	MainVolumeControl.h								\
	RemoteController.cpp							\
	RemoteController.h								\
	RemoteControllerProfiles.cpp					\
	RemoteControllerProfiles.h						\
	PowerStateMachine.cpp							\
	PowerStateMachine.h								\
	ConnObserverFile.cpp							\
	ConnObserverFile.h								\
	SoundCardSetup.cpp								\
	SoundCardSetup.h								\
	GPIOController.cpp								\
	GPIOController.h								\
	AudioSources/AbstractAudioSource.cpp			\
	AudioSources/AbstractAudioSource.h				\
	AudioSources/TrackChangeTransition.cpp			\
	AudioSources/TrackChangeTransition.h			\
	AudioSources/MPDAudioSource.cpp					\
	AudioSources/MPDAudioSource.h					\
	AudioSources/DLNAAudioSource.cpp				\
	AudioSources/DLNAAudioSource.h					\
	AudioSources/LMCAudioSource.cpp					\
	AudioSources/LMCAudioSource.h					\
	AudioSources/SourceMuteRampCtrl.cpp				\
	AudioSources/SourceMuteRampCtrl.h				\
	AudioSources/RetroradioAudioSourceList.cpp		\
	AudioSources/RetroradioAudioSourceList.h


retroradio_controller_CPPFLAGS = \
		-I generated			\
		$(GIO_LIBS_CFLAGS)				\
		$(GIO_UNIX_CFLAGS)				\
		$(GLIB_CFLAG)					\
		$(GOBJECT_CFLAGS)				\
		$(MPDC_CFLAGS)					\
		$(UDEV_CFLAGS)					\
		$(ALSA_CFLAGS)


retroradio_controller_LDADD	  = \
		-lCppAppUtils			\
		$(GIO_LIBS_LIBS)		\
		$(GIO_UNIX_LIBS)		\
		$(GLIB_LIBS)			\
		$(GOBJECT_LIBS)			\
		$(MPDC_LIBS)			\
		$(UDEV_LIBS)			\
		$(ALSA_LIBS)

