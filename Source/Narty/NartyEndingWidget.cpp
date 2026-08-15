#include "NartyEndingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

void UNartyEndingWidget::SetEndingTexts(const FText& Title, const FText& Body)
{
	CachedTitle = Title;
	CachedBody = Body;
	if (TitleText)
	{
		TitleText->SetText(CachedTitle);
	}
	if (BodyText)
	{
		BodyText->SetText(CachedBody);
	}
}

TSharedRef<SWidget> UNartyEndingWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UNartyEndingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (PlayAgainButton)
	{
		PlayAgainButton->OnClicked.AddDynamic(this, &UNartyEndingWidget::HandlePlayAgain);
	}
	SetEndingTexts(CachedTitle, CachedBody);
}

void UNartyEndingWidget::BuildLayout()
{
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.01f, 0.01f, 0.02f, 0.88f));
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
	Panel->SetBrushColor(FLinearColor(0.08f, 0.07f, 0.05f, 0.98f));
	Panel->SetPadding(FMargin(32.f, 28.f));
	PanelSize->SetContent(Panel);

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
	Panel->SetContent(Root);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	TitleText->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 30;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.82f, 0.4f)));
	if (UVerticalBoxSlot* TitleSlot = Root->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
	}

	BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Body"));
	BodyText->SetAutoWrapText(true);
	BodyText->SetJustification(ETextJustify::Center);
	FSlateFontInfo BodyFont = BodyText->GetFont();
	BodyFont.Size = 16;
	BodyText->SetFont(BodyFont);
	BodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.9f, 0.85f)));
	if (UVerticalBoxSlot* BodySlot = Root->AddChildToVerticalBox(BodyText))
	{
		BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
	}

	PlayAgainButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PlayAgain"));
	UTextBlock* AgainLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AgainLabel"));
	AgainLabel->SetText(NSLOCTEXT("Narty", "Ending_PlayAgain", "\u0418\u0433\u0440\u0430\u0442\u044c \u0441\u043d\u043e\u0432\u0430"));
	AgainLabel->SetJustification(ETextJustify::Center);
	AgainLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	PlayAgainButton->SetContent(AgainLabel);
	Root->AddChildToVerticalBox(PlayAgainButton);

	CachedTitle = NSLOCTEXT("Narty", "Ending_DefaultTitle", "Narty");
	CachedBody = FText::GetEmpty();
}

void UNartyEndingWidget::HandlePlayAgain()
{
	OnPlayAgain.Broadcast();
}
