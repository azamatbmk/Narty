#include "NartyInteractPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

void UNartyInteractPromptWidget::SetPromptVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

TSharedRef<SWidget> UNartyInteractPromptWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UNartyInteractPromptWidget::BuildLayout()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size"));
	Size->SetWidthOverride(220.f);
	if (UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size))
	{
		SizeSlot->SetHorizontalAlignment(HAlign_Center);
		SizeSlot->SetVerticalAlignment(VAlign_Bottom);
		SizeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 72.f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.04f, 0.65f));
	Panel->SetPadding(FMargin(14.f, 8.f));
	Size->SetContent(Panel);

	PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Prompt"));
	PromptText->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = PromptText->GetFont();
	Font.Size = 16;
	PromptText->SetFont(Font);
	PromptText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.88f, 0.55f)));
	PromptText->SetText(NSLOCTEXT("Narty", "Interact_Prompt", "E \u2014 \u0432\u0437\u0430\u0438\u043c\u043e\u0434\u0435\u0439\u0441\u0442\u0432\u0438\u0435"));
	Panel->SetContent(PromptText);

	SetVisibility(ESlateVisibility::Collapsed);
}
