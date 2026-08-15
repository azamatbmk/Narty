#include "NartyChoiceDialogWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

void UNartyChoiceDialogWidget::Setup(const FText& Title, const FText& Body, const TArray<FText>& Choices, bool bAllowDismiss)
{
	CachedTitle = Title;
	CachedBody = Body;
	CachedChoices = Choices;
	bCachedAllowDismiss = bAllowDismiss;

	if (TitleText)
	{
		TitleText->SetText(CachedTitle);
	}
	if (BodyText)
	{
		BodyText->SetText(CachedBody);
	}

	for (int32 i = 0; i < ChoiceLabels.Num(); ++i)
	{
		const bool bVisible = CachedChoices.IsValidIndex(i);
		if (ChoiceButtons.IsValidIndex(i) && ChoiceButtons[i])
		{
			ChoiceButtons[i]->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
		if (bVisible && ChoiceLabels[i])
		{
			ChoiceLabels[i]->SetText(CachedChoices[i]);
		}
	}

	if (CloseButton)
	{
		CloseButton->SetVisibility(bCachedAllowDismiss ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

TSharedRef<SWidget> UNartyChoiceDialogWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UNartyChoiceDialogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ChoiceButtons.IsValidIndex(0) && ChoiceButtons[0])
	{
		ChoiceButtons[0]->OnClicked.AddDynamic(this, &UNartyChoiceDialogWidget::HandleChoice0);
	}
	if (ChoiceButtons.IsValidIndex(1) && ChoiceButtons[1])
	{
		ChoiceButtons[1]->OnClicked.AddDynamic(this, &UNartyChoiceDialogWidget::HandleChoice1);
	}
	if (ChoiceButtons.IsValidIndex(2) && ChoiceButtons[2])
	{
		ChoiceButtons[2]->OnClicked.AddDynamic(this, &UNartyChoiceDialogWidget::HandleChoice2);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UNartyChoiceDialogWidget::HandleClose);
	}

	Setup(CachedTitle, CachedBody, CachedChoices, bCachedAllowDismiss);
}

void UNartyChoiceDialogWidget::BuildLayout()
{
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.02f, 0.78f));
	WidgetTree->RootWidget = Backdrop;

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay"));
	Backdrop->SetContent(Overlay);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	PanelSize->SetWidthOverride(560.f);
	if (UOverlaySlot* PanelSlot = Overlay->AddChildToOverlay(PanelSize))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.09f, 0.07f, 0.05f, 0.97f));
	Panel->SetPadding(FMargin(28.f, 22.f));
	PanelSize->SetContent(Panel);

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
	Panel->SetContent(Root);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	TitleText->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 26;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.82f, 0.45f)));
	if (UVerticalBoxSlot* TitleSlot = Root->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Body"));
	BodyText->SetAutoWrapText(true);
	FSlateFontInfo BodyFont = BodyText->GetFont();
	BodyFont.Size = 15;
	BodyText->SetFont(BodyFont);
	BodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.9f, 0.85f)));
	if (UVerticalBoxSlot* BodySlot = Root->AddChildToVerticalBox(BodyText))
	{
		BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	}

	for (int32 i = 0; i < 3; ++i)
	{
		AddChoiceButton(Root, i);
	}

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Close"));
	UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseLabel"));
	CloseLabel->SetText(NSLOCTEXT("Narty", "Choice_Later", "\u041f\u043e\u0437\u0436\u0435"));
	CloseLabel->SetJustification(ETextJustify::Center);
	CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CloseButton->SetContent(CloseLabel);
	if (UVerticalBoxSlot* CloseSlot = Root->AddChildToVerticalBox(CloseButton))
	{
		CloseSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	CachedTitle = NSLOCTEXT("Narty", "Choice_DefaultTitle", "Narty");
	CachedBody = FText::GetEmpty();
}

UButton* UNartyChoiceDialogWidget::AddChoiceButton(UVerticalBox* Root, int32 Index)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetAutoWrapText(true);
	Label->SetJustification(ETextJustify::Center);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = 15;
	Label->SetFont(Font);
	Button->SetContent(Label);

	if (UVerticalBoxSlot* BoxSlot = Root->AddChildToVerticalBox(Button))
	{
		BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	ChoiceButtons.Add(Button);
	ChoiceLabels.Add(Label);
	return Button;
}

void UNartyChoiceDialogWidget::HandleChoice0() { Pick(0); }
void UNartyChoiceDialogWidget::HandleChoice1() { Pick(1); }
void UNartyChoiceDialogWidget::HandleChoice2() { Pick(2); }

void UNartyChoiceDialogWidget::HandleClose()
{
	if (!bCachedAllowDismiss)
	{
		return;
	}
	OnClosed.Broadcast();
}

void UNartyChoiceDialogWidget::Pick(int32 Index)
{
	OnChoicePicked.Broadcast(Index);
}
