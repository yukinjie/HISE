/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 4.3.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_BBD5AE674E02BB6A__
#define __JUCE_HEADER_BBD5AE674E02BB6A__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class AudioFileEnvelopeEditor  : public ProcessorEditorBody,
                                 public Timer,
                                 public AudioDisplayComponent::Listener,
                                 public SliderListener,
                                 public ButtonListener,
                                 public ComboBoxListener
{
public:
    //==============================================================================
    AudioFileEnvelopeEditor (ProcessorEditor *p);
    ~AudioFileEnvelopeEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	int getBodyHeight() const override {return h;};

#pragma warning( push )
#pragma warning( disable: 4100 )

	void rangeChanged(AudioDisplayComponent *broadcaster, int changedArea)
	{
		jassert(broadcaster == sampleBufferContent);

		Range<int> newRange = sampleBufferContent->getSampleArea(changedArea)->getSampleRange();

		AudioSampleProcessor *envelope = dynamic_cast<AudioSampleProcessor*>(getProcessor());

		envelope->setLoadedFile(sampleBufferContent->getCurrentlyLoadedFileName());
		envelope->setRange(newRange);
	};

#pragma warning( pop )

	void updateGui() override
	{
		AudioFileEnvelope::EnvelopeFollowerMode mode = (AudioFileEnvelope::EnvelopeFollowerMode)(int)getProcessor()->getAttribute(AudioFileEnvelope::Mode);

		attackSlider->updateValue();
		releaseSlider->updateValue();
		modeSelector->updateValue();
		retriggerButton->updateValue();
		gainSlider->updateValue();
		smoothSlider->updateValue();
		offsetSlider->updateValue();
		rampSlider->updateValue();

		AudioFileEnvelope *envelope = dynamic_cast<AudioFileEnvelope*>(getProcessor());

		if(sampleBufferContent->getSampleArea(0)->getSampleRange() != envelope->getRange())
		{
			sampleBufferContent->setRange(envelope->getRange());


		}

		switch(mode)
		{
		case AudioFileEnvelope::SimpleLP:	attackSlider->setVisible(false);
											releaseSlider->setVisible(false);
											rampSlider->setVisible(false);
											smoothSlider->setVisible(true);
											gainSlider->setVisible(true);
											break;
		case AudioFileEnvelope::RampedAverage:	attackSlider->setVisible(false);
											releaseSlider->setVisible(false);
											rampSlider->setVisible(true);
											break;
		case AudioFileEnvelope::AttackRelease:	attackSlider->setVisible(true);
											releaseSlider->setVisible(true);
											rampSlider->setVisible(false);
											break;


		}
	}

	void timerCallback() override
	{
		sampleBufferContent->setPlaybackPosition(getProcessor()->getInputValue());
	};

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> label;
    ScopedPointer<HiSlider> smoothSlider;
    ScopedPointer<HiToggleButton> retriggerButton;
    ScopedPointer<AudioSampleProcessorBufferComponent> sampleBufferContent;
    ScopedPointer<HiComboBox> modeSelector;
    ScopedPointer<HiSlider> gainSlider;
    ScopedPointer<HiSlider> attackSlider;
    ScopedPointer<HiSlider> releaseSlider;
    ScopedPointer<HiSlider> offsetSlider;
    ScopedPointer<HiSlider> rampSlider;
    ScopedPointer<HiComboBox> syncToHost;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFileEnvelopeEditor)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_BBD5AE674E02BB6A__
