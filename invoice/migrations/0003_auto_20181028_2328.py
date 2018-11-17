# Generated by Django 2.1.2 on 2018-10-28 23:28

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('invoice', '0002_auto_20181013_1620'),
    ]

    operations = [
        migrations.AlterField(
            model_name='invoice',
            name='basis_calculation_icms',
            field=models.FloatField(blank=True),
        ),
        migrations.AlterField(
            model_name='invoice',
            name='basis_calculation_icms_st',
            field=models.FloatField(blank=True),
        ),
        migrations.AlterField(
            model_name='invoice',
            name='discount_value',
            field=models.FloatField(blank=True),
        ),
        migrations.AlterField(
            model_name='invoice',
            name='freight_value',
            field=models.FloatField(blank=True),
        ),
        migrations.AlterField(
            model_name='invoice',
            name='icms_value',
            field=models.FloatField(blank=True),
        ),
        migrations.AlterField(
            model_name='invoice',
            name='icms_value_st',
            field=models.FloatField(blank=True),
        ),
        migrations.AlterField(
            model_name='invoice',
            name='insurance_value',
            field=models.FloatField(blank=True),
        ),
        migrations.AlterField(
            model_name='invoice',
            name='ipi_value',
            field=models.FloatField(blank=True),
        ),
        migrations.AlterField(
            model_name='invoice',
            name='other_expenditure',
            field=models.FloatField(blank=True),
        ),
    ]
