#include "NartyObjectiveWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

void UNartyObjectiveWidget::SetObjective(const FText& Title, const FText& Detail)
{
	if (TitleText)
	{
		TitleText->SetText(Title);
	}
	if (DetailText)
	{
		DetailText->SetText(Detail);
	}
}

TSharedRef<SWidget> UNartyObjectiveWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UNartyObjectiveWidget::BuildLayout()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size"));
	Size->SetWidthOverride(460.f);
	if (UOverlaySlot* SizeSlot = RootOverlay->AddChildToOverlay(Size))
	{
		SizeSlot->SetHorizontalAlignment(HAlign_Left);
		SizeSlot->SetVerticalAlignment(VAlign_Top);
		SizeSlot->SetPadding(FMargin(36.f, 36.f, 0.f, 0.f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.04f, 0.72f));
	Panel->SetPadding(FMargin(16.f, 12.f));
	Size->SetContent(Panel);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	Panel->SetContent(Box);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 18;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.78f, 0.35f)));
	TitleText->SetText(NSLOCTEXT("Narty", "Obj_DefaultTitle", "\u0417\u0430\u0434\u0430\u043d\u0438\u0435"));
	Box->AddChildToVerticalBox(TitleText);

	DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Detail"));
	DetailText->SetAutoWrapText(true);
	FSlateFontInfo DetailFont = DetailText->GetFont();
	DetailFont.Size = 14;
	DetailText->SetFont(DetailFont);
	DetailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.88f)));
	if (UVerticalBoxSlot* DetailSlot = Box->AddChildToVerticalBox(DetailText))
	{
		DetailSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}
}
