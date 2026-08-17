#include "NartyHealthWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

void UNartyHealthWidget::SetHealth(float Health, float MaxHealth)
{
	const float SafeMax = FMath::Max(1.f, MaxHealth);
	const float Ratio = FMath::Clamp(Health / SafeMax, 0.f, 1.f);

	if (HealthBar)
	{
		HealthBar->SetPercent(Ratio);
		const FLinearColor Color = Ratio > 0.35f
			? FLinearColor(0.72f, 0.18f, 0.16f)
			: FLinearColor(0.45f, 0.05f, 0.05f);
		HealthBar->SetFillColorAndOpacity(Color);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::Format(
			NSLOCTEXT("Narty", "HP_Format", "HP {0} / {1}"),
			FText::AsNumber(FMath::RoundToInt(Health)),
			FText::AsNumber(FMath::RoundToInt(MaxHealth))));
	}
}

TSharedRef<SWidget> UNartyHealthWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UNartyHealthWidget::BuildLayout()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size"));
	Size->SetWidthOverride(280.f);
	if (UOverlaySlot* SizeSlot = RootOverlay->AddChildToOverlay(Size))
	{
		SizeSlot->SetHorizontalAlignment(HAlign_Left);
		SizeSlot->SetVerticalAlignment(VAlign_Bottom);
		SizeSlot->SetPadding(FMargin(36.f, 0.f, 0.f, 36.f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.04f, 0.72f));
	Panel->SetPadding(FMargin(14.f, 10.f));
	Size->SetContent(Panel);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	Panel->SetContent(Box);

	HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
	FSlateFontInfo Font = HealthText->GetFont();
	Font.Size = 16;
	HealthText->SetFont(Font);
	HealthText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.9f, 0.82f)));
	Box->AddChildToVerticalBox(HealthText);

	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
	HealthBar->SetPercent(1.f);
	HealthBar->SetFillColorAndOpacity(FLinearColor(0.72f, 0.18f, 0.16f));
	if (UVerticalBoxSlot* BarSlot = Box->AddChildToVerticalBox(HealthBar))
	{
		BarSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	SetHealth(100.f, 100.f);
}
