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
		<label v-for="[value, words] in choices" :key="value" class="flex items-center gap-2"><!-- the words inside the label, so clicking them picks the answer too -->
			<input type="radio" :name="name" :value="value" :checked="value == chosen" @change="$emit('choose', value)" />
			{{ words }}
		</label>
	</fieldset>
</template>
