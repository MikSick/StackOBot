/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2024 Audiokinetic Inc.
*******************************************************************************/

#include "MovieSceneWwiseGameParameterTemplate.h"
#include "AkAudioDevice.h"
#include "MovieSceneWwiseGameParameterSection.h"
#include "MovieSceneExecutionToken.h"
#include "IMovieScenePlayer.h"


FMovieSceneWwiseGameParameterSectionData::FMovieSceneWwiseGameParameterSectionData(const UMovieSceneWwiseGameParameterSection& Section)
	: GameParameter(Section.GetGameParameter())
	, GameParameterChannel(Section.GetChannel())
{
}


struct FAkAudioRTPCEvaluationData : IPersistentEvaluationData
{
	TSharedPtr<FMovieSceneWwiseGameParameterSectionData> SectionData;
};


struct FAkAudioRTPCExecutionToken : IMovieSceneExecutionToken
{
	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		auto AudioDevice = FAkAudioDevice::Get();
		if (!AudioDevice)
		{
			return;
		}

		FAkAudioRTPCEvaluationData* EvaluationData = PersistentData.FindSectionData<FAkAudioRTPCEvaluationData>();
		if (EvaluationData && EvaluationData->SectionData.IsValid())
		{
			TSharedPtr<FMovieSceneWwiseGameParameterSectionData> SectionData = EvaluationData->SectionData;
			float Value;
			auto RTPC = SectionData->GameParameter;
			if (!SectionData->GameParameterChannel.Evaluate(Context.GetTime(), Value))
			{
				UE_LOG(LogAkAudio, Verbose, TEXT(" FAkAudioRTPCExecutionToken::Execute: Failed to get RTPC parameter at time %f. RTPC Value: %f"), Context.GetTime().AsDecimal(), Value)
				return;
			}

			if (Operand.ObjectBindingID.IsValid())
			{	// Object binding audio track
				for (auto ObjectPtr : Player.FindBoundObjects(Operand))
				{
					auto Object = ObjectPtr.Get();
					if (Object)
					{
						auto Actor = CastChecked<AActor>(Object);
						if (Actor)
						{
							AudioDevice->SetRTPCValue(RTPC, Value, 0, Actor);
						}
					}
				}
			}
			else
			{	// Master audio track
				AudioDevice->SetRTPCValue(RTPC, Value, 0, nullptr);
			}
		}
	}
};


FMovieSceneWwiseGameParameterTemplate::FMovieSceneWwiseGameParameterTemplate(const UMovieSceneWwiseGameParameterSection& InSection)
	: Section(&InSection) {}

void FMovieSceneWwiseGameParameterTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	ExecutionTokens.Add(FAkAudioRTPCExecutionToken());
}

void FMovieSceneWwiseGameParameterTemplate::Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	if (Section != nullptr && Section->GetGameParameter() != nullptr)
	{
		PersistentData.AddSectionData<FAkAudioRTPCEvaluationData>().SectionData = MakeShareable(new FMovieSceneWwiseGameParameterSectionData(*Section));
	}
}
