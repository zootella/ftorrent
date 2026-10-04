<script setup>
import {useId} from 'vue'

defineProps({
	choices:  Array,  //the answers, as [value, words] pairs, in the order shown
	chosen:   String, //the value checked now
	disabled: Boolean,//gray the whole question, answers and all
})
defineEmits(['choose'])//with the value of the answer the user picked

let name = useId()//one name shared by this group's buttons, which is what makes them a group: one checked at a time, and arrow keys moving among them. Unique per group, so two questions never share one
</script>

<template>
	<!-- ./src/components/RadioGroup.vue -->
	<!-- a question and every one of its answers in view at once, the way ftorrent asks for a choice, rather than with a dropdown that hides them. fieldset and legend name the group, so a screen reader says the question before the answers; the slot is the question -->
	<fieldset :disabled="disabled">
		<legend class="mb-1"><slot /></legend>
		<label v-for="[value, words] in choices" :key="value" class="flex w-fit items-center gap-2"><!-- the words inside the label, so clicking them picks the answer too; and only as wide as they are, since a flex label is a block that would otherwise stretch across the page, and a click in the empty space beside an answer, with nothing there to show it, would pick it -->
			<input type="radio" :name="name" :value="value" :checked="value == chosen" @change="$emit('choose', value)" />
			<span><slot name="answer" :words="words">{{ words }}</slot></span><!-- the words as they are, unless the page draws its answers its own way, as the font question does to set each face's name in italics. The span keeps them one flex item: the label is a flex row, so without it each run of text and each em would be an item of its own, with the row's gap between them -->
		</label>
	</fieldset>
</template>
